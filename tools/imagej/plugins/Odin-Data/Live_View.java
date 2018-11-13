import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.nio.*;

import javax.lang.model.util.ElementScanner6;

import org.json.*;

import ij.plugin.frame.*;
import ij.*;
import ij.process.*;
import ij.gui.*;

import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMQ;

/** This a prototype ImageJ plugin. */
public class Live_View extends PlugInFrame implements ActionListener
{
	private Panel panel;
	private int previousID;
	private static Frame instance;
	private ImagePlus img = null;
	
	private String socket_addr = "tcp://127.0.0.1:5020";

	public Live_View()
	{
		super("Live View Controls");
		if (instance!=null) {
			instance.toFront();
			return;
		}
		instance = this;
		addKeyListener(IJ.getInstance());
		setLayout(new FlowLayout());
		panel = buildGUI(instance);
		add(panel);
		
		pack();
		GUI.center(this);
		setVisible(true);
		
	}
	
	private Panel buildGUI(Frame instance)
	{
		panel = new Panel();
		panel.setLayout(new GridBagLayout());
		
		Label lbl_socket_addr = new Label("Socket Address");
		Label lbl_image_dims = new Label("Image Dimensions");
		Label lbl_dataset = new Label("Frame Dataset");
		Label lbl_live_on_off = new Label("Enable Live View");
		
		TextField txt_socket_addr = new TextField(socket_addr, 25);
		txt_socket_addr.setEditable(true);
		TextField txt_image_dims = new TextField("[0 , 0]", 12);
		txt_image_dims.setEditable(false);
		TextField txt_dataset = new TextField("None", 12);
		txt_dataset.setEditable(false);
		
		Button btn_live_on_off = new Button("OFF");
		
		GridBagConstraints constraints = new GridBagConstraints();
		constraints.insets = new Insets(2,2,2,2);

		//Top Row Stuff
		constraints.anchor = GridBagConstraints.CENTER;

		constraints.gridx = 0;  //Top Left corner. y is vertical, x is horizontal
		constraints.gridy = 0;
		panel.add(lbl_socket_addr, constraints);
		
		constraints.gridx = 1;
		panel.add(lbl_dataset, constraints);

		constraints.gridx = 2;
		panel.add(lbl_image_dims, constraints);

		constraints.gridx = 3;
		panel.add(lbl_live_on_off, constraints);

		//Next Row
		constraints.gridx = 0;
		constraints.gridy = 1;
		panel.add(txt_socket_addr, constraints);

		constraints.gridx = 1;
		panel.add(txt_dataset, constraints);

		constraints.gridx = 2;
		panel.add(txt_image_dims, constraints);

		constraints.gridx = 3;
		panel.add(btn_live_on_off, constraints);

		return panel;
	}

	public void run(String arg) 
	{
		long start = System.currentTimeMillis();
		int w = 400, h = 400;
		ImageProcessor ip = new ByteProcessor(w, h);
		byte[] pixels = (byte[])ip.getPixels();
		int i = 0;
		for (int y = 0; y < h; y++) {
			byte red = (byte)((y * 255) / (h - 1));
			for (int x = 0; x < w; x++) {
				
				pixels[i++] = red;
			}
		}
		img = new ImagePlus("Live View", ip);
		img.show();
		printMessage("Time Taken: "+(System.currentTimeMillis()-start));

		setup_socket(socket_addr);
	}

	public void run_socket(Socket socket, ZMQ.Context context)
	{
		while(!Thread.currentThread().isInterrupted())
        {
            ZMQ.Poller items = new ZMQ.Poller(1);
            items.register(socket, ZMQ.Poller.POLLIN);

            if(items.poll(0) == -1)
            {
                printMessage("POLL RETURN -1");
                break;
            }

            if(items.pollin(0))
            {
				recvFrame(socket);
                // printMessage("RECIVED MESSAGE: '" + message + "'");
            }
        }
        
        printMessage("Socket recieved an interrupt signal. Closing down socket");
        socket.close();
        context.term();
	}

	public void setup_socket(String socket_addr)
	{
		printMessage("CREATING ZMQ SOCKET");
		ZMQ.Context context = ZMQ.context(1);
		ZMQ.Socket subscriber = context.socket(ZMQ.SUB);
		subscriber.connect(socket_addr);
		subscriber.subscribe("".getBytes());
		printMessage(String.format("Subscribed to addr %s",socket_addr));

		Thread zmqThread = new Thread(){
			@Override
			public void run()
			{
				run_socket(subscriber, context);
			}
		};

		Runtime.getRuntime().addShutdownHook(new Thread(){
            @Override
            public void run(){
                printMessage("Interrupt received, shutting down plugin");
                
                try {
                    zmqThread.interrupt();
                    
                    zmqThread.join();
                } catch (Exception e) 
                {
                    printMessage("INTERRUPT ERROR: " + e.getMessage());
                    e.printStackTrace();
                }
            }
        });

        zmqThread.start();
	}

	public void printMessage(String message)
	{
		System.out.println(message);
		IJ.log(message);
	}

	public void actionPerformed(ActionEvent e)
	{
		printMessage("Action Performed: " + e.toString());
	}

	public void recvFrame(ZMQ.Socket socket)
	{

		JSONObject header = new JSONObject(new String(socket.recv()));
		printMessage(header.toString());
		ByteBuffer img_data = ByteBuffer.wrap(socket.recv());
		try
		{
			JSONArray JSONshape = header.optJSONArray("shape");
			int[] shape = new int[]{JSONshape.getInt(0), JSONshape.getInt(1)};

			String dtype = header.optString("dtype");

			refreshImage(img_data, dtype, shape);
			
		}
		catch(JSONException except)
		{
			printMessage("Error receiving header: "+ except.getMessage());
			return;
		}
		
	}

	private void refreshImage(ByteBuffer data, String dtype, int[] shape)
	{
		ImageProcessor ip = img.getProcessor();
		boolean need_new_processor = false;
		// Buffer img_data = null;
		Object img_pixels = null;

		//need to check if the shape or the data type of the image have changed. If they have, we need to create a new Processor
		int current_dtype = ip.getBitDepth();
		int[] current_shape = new int[]{ip.getWidth(), ip.getHeight()};

		if(current_shape[0] != shape[0] || current_shape[1] != shape[1])
		{
			need_new_processor = true;
			printMessage(String.format("Shape Does not match: current: [%d,%d], new: [%d,%d]", current_shape[0], current_shape[1], shape[0], shape[1]));
		}

		switch (current_dtype)
		{
			case 8:
				if(!dtype.equalsIgnoreCase("uint8"))
				{
					need_new_processor = true;
					printMessage(String.format("Data Type does not match. Current: uint8, new: %s", dtype));
				}
				else
				{
					// img_data = data;
				}
				break;

			case 16:
				if(!dtype.equalsIgnoreCase("uint16"))
				{
					need_new_processor = true;
					printMessage(String.format("Data Type does not match. Current: uint16, new: %s", dtype));
				}
				else
				{
					// img_data = data.asShortBuffer();
				}
				break;

			case 32:
				if(!dtype.equalsIgnoreCase("uint32") && !dtype.equalsIgnoreCase("float"))
				{
					need_new_processor = true;
				}
				else
				{
					// img_data = data.asFloatBuffer();
					
				}
				break;

			default: //we dont support 24 bit colour images
				need_new_processor = true;
				break;
		}

		if(need_new_processor)
		{

		}
		else
		{

			switch (dtype)
			{
				case "uint8":
					// ByteBuffer byteBuf = data;
					img_pixels = new byte[data.limit()];
					data.get((byte[])img_pixels);
					break;
					
				case "uint16":
					ShortBuffer shortBuf = data.asShortBuffer();
					img_pixels = new short[shortBuf.limit()];
					shortBuf.get((short[])img_pixels);
					break;

				case "float":
				case "uint32":
					FloatBuffer floatBuf = data.asFloatBuffer();
					img_pixels = new float[floatBuf.limit()];
					floatBuf.get((float[])img_pixels);
					break;
			}

			ip.setPixels(img_pixels);
			img.updateAndDraw();
			img.updateStatusbarValue();
		}
	}
}