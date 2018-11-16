import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.nio.*;
import java.util.HashMap;
import java.util.Map;

import org.json.*;

import ij.plugin.frame.*;
import ij.*;
import ij.process.*;
import ij.gui.*;

import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMQ;
import org.zeromq.ZMQException;

/** This a prototype ImageJ plugin. */
public class Live_View extends PlugInFrame implements ActionListener
{
	private Panel panel;
	TextField txt_socket_addr;
	TextField txt_image_dims;
	TextField txt_dataset;
	TextField txt_status;

	private static Frame instance;
	
	private String socket_addr = "tcp://127.0.0.1:5020";

	private Map<String, Integer> dtype_map;

	LiveViewSocket socket = null;

	public Live_View()
	{
		super("Live View Controls");
		if (instance!=null) {
			instance.toFront();
			return;
		}
		
		dtype_map = new HashMap<String, Integer>();
		dtype_map.put("uint8", 8);
		dtype_map.put("uint16", 16);
		dtype_map.put("uint32", 32);
		dtype_map.put("float", 32);

		instance = this;
		addKeyListener(IJ.getInstance());
		setLayout(new FlowLayout());
		panel = buildGUI(instance);
		add(panel);
		
		pack();

		//add listener to window close events to shutdown socket
		addWindowListener(new WindowAdapter() 
		{
			public void windowClosing(WindowEvent e)
			{
				printMessage("Window Closing");
				if(socket != null)
				{
					socket.shutdown_socket();
				}
			}
		});

		Runtime.getRuntime().addShutdownHook(new Thread()
		{
			@Override
			public void run(){
				// printMessage("Interrupt received, shutting down plugin");
				try
				{
					socket.shutdown_socket();
				} 
				catch (Exception e) 
				{
					printMessage("INTERRUPT ERROR: " + e.getMessage());
				}
			}
		});
	}

	private Panel buildGUI(Frame instance)
	{
		panel = new Panel();
		panel.setLayout(new GridBagLayout());
		
		Label lbl_socket_addr = new Label("Socket Address");
		Label lbl_image_dims = new Label("Image Dimensions");
		Label lbl_dataset = new Label("Frame Dataset");
		Label lbl_live_on_off = new Label("Enable Live View");
		Label lbl_status = new Label("Status:");
		
		txt_socket_addr = new TextField(socket_addr, 25);
		txt_socket_addr.setEditable(true);
		txt_socket_addr.setBackground(Color.red);
		txt_image_dims = new TextField("[0 , 0]", 12);
		txt_image_dims.setEditable(false);
		txt_dataset = new TextField("None", 12);
		txt_dataset.setEditable(false);
		txt_status = new TextField("", 40);
		txt_status.setEditable(false);
		
		Button btn_live_on_off = new Button("PAUSE");
		Button btn_connect = new Button("CONNECT"); 
		
		//constraints for components.
		GridBagConstraints constraints = new GridBagConstraints();
		constraints.insets = new Insets(2,2,2,2);
		constraints.anchor = GridBagConstraints.CENTER;

		//top row
		addComponent(lbl_socket_addr, panel, 0, 0, constraints);

		addComponent(lbl_dataset, panel,     2, 0, constraints);
		addComponent(lbl_image_dims, panel,  3, 0, constraints);
		addComponent(lbl_live_on_off, panel, 4, 0, constraints);

		//middle row
		addComponent(txt_socket_addr, panel, 0, 1, constraints);
		addComponent(btn_connect, panel,     1, 1, constraints);
		addComponent(txt_dataset, panel,     2, 1, constraints);
		addComponent(txt_image_dims, panel,  3, 1, constraints);
		addComponent(btn_live_on_off, panel, 4, 1, constraints);

		//bottom row
		constraints.anchor = GridBagConstraints.EAST;
		addComponent(lbl_status, panel, 0, 2, constraints);
		constraints.anchor = GridBagConstraints.WEST;
		constraints.gridwidth = 3;		
		addComponent(txt_status, panel, 1, 2, constraints);

		//attatching event listeners
		btn_live_on_off.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent event)
			{
				boolean is_running = socket.invert_is_running();
				
				if(is_running)
				{
					btn_live_on_off.setLabel("PAUSE");
					// txt_socket_addr.setBackground(Color.green);
				}
				else
				{
					btn_live_on_off.setLabel("UNPAUSE");
					// txt_socket_addr.setBackground(Color.red);
				}
			}

		});

		btn_connect.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent event)
			{
				if(socket == null)
				{
					try
					{
					socket = new LiveViewSocket(socket_addr);
					btn_connect.setLabel("DISCONNECT");
					txt_socket_addr.setBackground(Color.green);
					txt_socket_addr.setEditable(false);
					}
					catch(ZMQException except)
					{
						return;
					}
				}
				else
				{
					socket.shutdown_socket();
					socket = null;
					btn_connect.setLabel("CONNECT");
					txt_socket_addr.setEditable(true);
					txt_socket_addr.setBackground(Color.red);
				}
			}
		});

		txt_socket_addr.addFocusListener(new FocusListener()
		{
			public void focusLost(FocusEvent event)
			{
				socket_addr = txt_socket_addr.getText();
				printMessage("SOCKET ADDR CHANGED");
			}
			//has to have this method even if we dont want to do anything on focus Gained
			public void focusGained(FocusEvent event){}
		});

		return panel;
		
	}

	//Add a component to the supplied panel, using the suppiled constraints, at the grid position (x,y)
	private void addComponent(Component item, Panel panel, int x, int y, GridBagConstraints c)
	{
		c.gridx = x;
		c.gridy = y;
		panel.add(item, c);
	}

	public void run(String arg) 
	{
		GUI.center(this);
		setVisible(true);
		// socket = new LiveViewSocket(socket_addr);	
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


	private class LiveViewSocket 
	{
		private ZMQ.Socket socket;
		private ZMQ.Context context;
		private ImagePlus img = null;

		private boolean is_running = true;

		private Thread zmqThread;

		public LiveViewSocket(String socket_addr) throws ZMQException
		{
			printMessage("CREATING ZMQ SOCKET");
			context = ZMQ.context(1);
			socket = context.socket(ZMQ.SUB);
			try
			{
				socket.connect(socket_addr);
			}
			catch(ZMQException except)
			{
				printMessage("EXCEPTION CONNECTING SOCKET");
				throw except;
				
			}
			
			socket.subscribe("".getBytes());
			printMessage(String.format("Subscribed to addr %s",socket_addr));

			img = new ImagePlus();
	
			zmqThread = new Thread(){
				@Override
				public void run()
				{
					run_socket();
				}
			};
		
			zmqThread.start();
		}
		   
		private void run_socket()
		{
			while(!Thread.currentThread().isInterrupted())
			{
				if(is_running)
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
						recvFrame();
					}
				}		
			}
			
			// shutdown_socket();
		}

		public void shutdown_socket()
		{
			printMessage("Socket Shutdown Signal Received. Closing socket");
			zmqThread.interrupt();
			socket.close();
			context.close();
			img.hide();

		}

		private void recvFrame()
		{
			//Get header from first ZMQ message, turn it into a JSON Object
			JSONObject header = new JSONObject(new String(socket.recv()));
			ByteBuffer img_data = ByteBuffer.wrap(socket.recv());
			printMessage(header.toString());
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
			boolean need_new_processor = false;
			int bitdepth = dtype_map.get(dtype);
			ImageProcessor ip = img.getProcessor();
			Object img_pixels = null;
			
			//image window may have been closed by user, which causes null pointer exceptions if trying to get the bit depth. 
			if(!img.isVisible())
			{
				need_new_processor = true;
			}
			txt_image_dims.setText(String.format("[%d, %d]", shape[0], shape[1]));
	
			//need to check if the shape or the data type of the image have changed. If they have, we need to create a new Processor
			
			
			if(!need_new_processor)
			{
				int current_bitdepth = ip.getBitDepth();
				int[] current_shape = new int[]{ip.getWidth(), ip.getHeight()};
				if(current_shape[0] != shape[0] || current_shape[1] != shape[1])
				{
					need_new_processor = true;
					printMessage(String.format("Shape Does not match: current: [%d,%d], new: [%d,%d]", current_shape[0], current_shape[1], shape[0], shape[1]));
				}
				else
				if(bitdepth != current_bitdepth)
				{
					need_new_processor = true;
				}
			}
	
			switch (bitdepth)
			{
				case 8:
					if(need_new_processor)
					{
						ip = new ByteProcessor(shape[0], shape[1]);
						img.setProcessor(ip);
					}
					img_pixels = new byte[data.limit()];
					data.get((byte[])img_pixels);
					break;
	
				case 16:
					if(need_new_processor)
					{
						ip = new ShortProcessor(shape[0], shape[1]);
						img.setProcessor(ip);
					}
					ShortBuffer shortBuf = data.asShortBuffer();
					img_pixels = new short[shortBuf.limit()];
					shortBuf.get((short[])img_pixels);
					break;
	
				case 32:
					if(need_new_processor){
						ip = new FloatProcessor(shape[0], shape[1]);
						img.setProcessor(ip);
					}
					FloatBuffer floatBuf = data.asFloatBuffer();
					img_pixels = new float[floatBuf.limit()];
					floatBuf.get((float[])img_pixels);
					break;
			}

			ip.setPixels(img_pixels);
			img.updateAndDraw();
			img.updateStatusbarValue();
			if(!img.isVisible())
			{
				img.setTitle("Live View From: "+ socket_addr);
				img.show();
			}
		}

		public boolean invert_is_running()
		{
			is_running = !is_running;
			return is_running;
		}

		public boolean is_running()
		{
			return is_running;
		}
	}//Class LiveViewSocket
}