import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.nio.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import org.json.*;

import ij.plugin.frame.*;
import ij.*;
import ij.process.*;
import ij.gui.*;

import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMQ;
import org.zeromq.ZMQException;

/**
 * This is an ImageJ Plugin that allows ImageJ to receive and display images from the Odin Data Live View Plugin.
 * @author Adam Neaves
 * @version 0.1.0
 */
public class Live_View extends PlugInFrame implements ActionListener
{
	private Panel panel;
	TextField txt_socket_addr;
	TextField txt_image_dims;
	TextField txt_dataset;
	TextField txt_status;
	TextField txt_fps;

	private static Frame instance;
	
	private String socket_addr = "tcp://127.0.0.1:5020";

	private Map<String, Integer> dtype_map;

	LiveViewSocket socket = null;

	boolean is_logging = false;

	/**
	 * Constructs the Live View Plugin.
	 * It creates and displays the Control GUI and sets up required objects.
	 * It also sets up the action listeners that listen for the window closing or being shut down, so it
	 * can tear down cleanly.
	 */
	public Live_View()
	{
		super("Live View Controls");
		if (instance!=null) {
			instance.setVisible(true);
			instance.toFront();
			return;
		}
		
		dtype_map = new HashMap<String, Integer>();
		dtype_map.put("uint8", 8);
		dtype_map.put("uint16", 16);
		dtype_map.put("uint32", 32);
		dtype_map.put("float", 33);

		instance = this;
		addKeyListener(IJ.getInstance());
		setLayout(new FlowLayout());
		panel = buildGUI();
		add(panel);
		
		pack();
		GUI.center(this);
		setVisible(true);

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
				instance = null;
			}
		});

		Runtime.getRuntime().addShutdownHook(new Thread()
		{
			@Override
			public void run(){
				// printMessage("Interrupt received, shutting down plugin");
				instance = null;
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

	/**
	 * Creates the panel containing the control GUI components.
	 * It also adds the event listeners required for some of the
	 * buttons and editable text fields.
	 * @return the constructed panel with all components added.
	 */
	private Panel buildGUI()
	{
		panel = new Panel();
		panel.setLayout(new GridBagLayout());
		
		Label lbl_socket_addr = new Label("Socket Address");
		Label lbl_image_dims = new Label("Image Dimensions");
		Label lbl_dataset = new Label("Frame Dataset");
		Label lbl_live_on_off = new Label("Enable Live View");
		Label lbl_status = new Label("Status:");
		Label lbl_logging = new Label("Log To Window");
		Label lbl_fps = new Label("Frames/s");

		txt_socket_addr = new TextField(socket_addr, 25);
		txt_socket_addr.setEditable(true);
		txt_socket_addr.setBackground(Color.red);
		txt_socket_addr.setForeground(Color.white);
		txt_image_dims = new TextField("[0 , 0]", 12);
		txt_image_dims.setEditable(false);
		txt_dataset = new TextField("None", 12);
		txt_dataset.setEditable(false);
		txt_status = new TextField("", 79);
		txt_status.setEditable(false);
		txt_fps = new TextField("", 8);
		txt_fps.setEditable(false);
		
		Button btn_live_on_off = new Button("  PAUSE  ");
		Button btn_connect = new Button(" CONNECT  ");
		Checkbox chk_logging = new Checkbox();
		chk_logging.setState(is_logging);
		
		//constraints for components.
		GridBagConstraints constraints = new GridBagConstraints();
		constraints.insets = new Insets(2,2,2,2);
		constraints.anchor = GridBagConstraints.CENTER;

		//top row
		addComponent(lbl_socket_addr, panel, 0, 0, constraints);

		addComponent(lbl_dataset, panel,     2, 0, constraints);
		addComponent(lbl_image_dims, panel,  3, 0, constraints);
		addComponent(lbl_fps, panel,         4, 0, constraints);
		addComponent(lbl_logging, panel,     5, 0, constraints);
		addComponent(lbl_live_on_off, panel, 6, 0, constraints);

		//middle row
		addComponent(txt_socket_addr, panel, 0, 1, constraints);
		addComponent(btn_connect, panel,     1, 1, constraints);
		addComponent(txt_dataset, panel,     2, 1, constraints);
		addComponent(txt_image_dims, panel,  3, 1, constraints);
		addComponent(txt_fps, panel,         4, 1, constraints);
		addComponent(chk_logging, panel,     5, 1, constraints);
		addComponent(btn_live_on_off, panel, 6, 1, constraints);

		//bottom row
		constraints.anchor = GridBagConstraints.EAST;
		addComponent(lbl_status, panel, 0, 2, constraints);
		constraints.anchor = GridBagConstraints.WEST;
		constraints.gridwidth = 6;		
		addComponent(txt_status, panel, 1, 2, constraints);

		//attatching event listeners
		btn_live_on_off.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent event)
			{
				if(socket != null){
					boolean is_paused = socket.invert_paused();		
					if(!is_paused)
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
					txt_socket_addr.setForeground(Color.black);
					txt_socket_addr.setEditable(false);
					}
					catch(ZMQException except)
					{
						IJ.error("ERROR CONNECTING SOCKET", "Check if the socket address is valid.");
					}
				}
				else
				{
					socket.shutdown_socket();
					socket = null;
					btn_connect.setLabel("CONNECT");
					txt_socket_addr.setEditable(true);
					txt_socket_addr.setBackground(Color.red);
					txt_socket_addr.setForeground(Color.white);
				}
			}
		});

		txt_socket_addr.addFocusListener(new FocusListener()
		{
			public void focusLost(FocusEvent event)
			{
				if(!txt_socket_addr.getText().equals(socket_addr))
				{
					socket_addr = txt_socket_addr.getText();
					printMessage("SOCKET ADDR CHANGED");
				}
			}
			//has to have this method even if we dont want to do anything on focus Gained
			public void focusGained(FocusEvent event){}
		});

		chk_logging.addItemListener(new ItemListener() {
			public void itemStateChanged(ItemEvent e) {             
			   is_logging = chk_logging.getState();
			}
		 });

		return panel;
		
	}

	/**
	 * Add a component to the supplied panel, using the suppiled constraints, at the grid position (x,y).
	 * @param item  the component being added
	 * @param panel the panel to add the compoent to
	 * @param x     the horizontal position on the grid where the component should be placed.
	 * @param y     the vertical position on the grid where the component should be placed.
	 * @param c     the constraints used for the item.
	 */
	private void addComponent(Component item, Panel panel, int x, int y, GridBagConstraints c)
	{
		c.gridx = x;
		c.gridy = y;
		panel.add(item, c);
	}

	/**
	 * Print a message to the status bar, and also potentionally to the console and log window.
	 * Adds a timestamp to the front of the message.
	 * @param message the string to print. usually some debug or status information
	 */
	public void printMessage(String message)
	{
		Date date = new Date();
		SimpleDateFormat simple_date = new SimpleDateFormat("yyyy-MM-dd kk:mm:ss.SSS");

		String full_message = simple_date.format(date) + ": " + message;
		IJ.showStatus(full_message);
		txt_status.setText(full_message);
		if(is_logging)
		{
			System.out.println(full_message);
			IJ.log(full_message);
		}
	}

	public void actionPerformed(ActionEvent e)
	{
		printMessage("Action Performed: " + e.toString());
	}


	/**
	 * Class that contains the ZMQ subscriber socket and logic for what to do with an image when it arrives.
	 * @author Adam Neaves
	 * @version 0.1.0
	 */
	private class LiveViewSocket 
	{
		private ZMQ.Socket socket;
		private ZMQ.Context context;
		private ImagePlus img = null;

		private boolean is_paused = false;

		private Thread zmqThread;

		private TimerTask status_update;
		private Timer frame_counter;

		long time_last_image = System.currentTimeMillis();
		int image_count = 0;

		/**
		 * Constructor for the class. Sets up threads that run the ZMQ socket and background timer which counts the frames.
		 * @param socket_addr    the address of the live view that the socket needs to subscribe to.
		 * @throws ZMQException  if the socket address is invalid or malformed
		 */
		public LiveViewSocket(String socket_addr) throws ZMQException
		{
			setup_socket(socket_addr);
			img = new ImagePlus();
	
			zmqThread = new Thread(){
				@Override
				public void run()
				{
					run_socket();
				}
			};

			status_update = new TimerTask(){
				long prev_time = System.currentTimeMillis();
				@Override
				public void run() {
					if(!is_paused)
					{
						long start_time = System.currentTimeMillis();
						float elapsed_time = (start_time - prev_time)/1000f;
						float fps = image_count / elapsed_time;
						
						txt_fps.setText(String.format("%.2f fps", fps));
						printMessage(String.format("Received %d images in %.3f seconds", image_count, elapsed_time));
						prev_time = start_time;
						image_count = 0;
					}
				}
			};
			frame_counter = new Timer(true);
			int timer_delay = 2000;
			frame_counter.schedule(status_update, 0, timer_delay);
			

			zmqThread.start();
		}
		
		/**
		 * Repeatedly Polls the ZMQ socket and reacts to incoming messages.
		 */
		private void run_socket()
		{
			while(!Thread.currentThread().isInterrupted())
			{
				if(!is_paused)
				{
					ZMQ.Poller items = new ZMQ.Poller(1);
					items.register(socket, ZMQ.Poller.POLLIN);

					if(items.poll(0) == -1)
					{
						break;
					}

					if(items.pollin(0)) //something has arrived at the socket, need to read it.
					{
						recvFrame();
					}
				}		
			}
			
			// shutdown_socket();
		}

		/**
		 * Closes down the socket and context, allowing the plugin to close cleanly.
		 * Interrupts the thread running the socket.
		 */
		public void shutdown_socket()
		{
			printMessage("Socket Shutdown Signal Received. Closing socket");
			zmqThread.interrupt();
			socket.close();
			context.close();
			img.hide();
			frame_counter.cancel();
		}

		/**
		 * Sets up the socket, subscribing it to the supplied socket address
		 * @param addr the socket address to subscribe to.
		 * @throws ZMQException if the socket address supplied is malformed or otherwise invalid.
		 */
		public void setup_socket(String addr) throws ZMQException
		{
			printMessage("CREATING ZMQ SOCKET");
			context = ZMQ.context(1);
			socket = context.socket(ZMQ.SUB);
			try
			{
				socket.connect(addr);
			}
			catch(ZMQException except)
			{
				printMessage("EXCEPTION CONNECTING SOCKET");
				throw except;
				
			}
			
			socket.subscribe("".getBytes());
			printMessage(String.format("Subscribed to addr %s",socket_addr));
			return;
		}

		/**
		 * Receives a frame from the ZMQ socket. The frame consists of a JSON header message,
		 * and then a message containing a blob of binary data for the image itself.
		 */
		private void recvFrame()
		{
			//Get header from first ZMQ message, turn it into a JSON Object
			JSONObject header = new JSONObject(new String(socket.recv()));
			ByteBuffer img_data = ByteBuffer.wrap(socket.recv());
			try
			{
				JSONArray JSONshape = header.optJSONArray("shape");
				int[] shape = new int[]{JSONshape.getInt(1), JSONshape.getInt(0)};

				String dtype = header.optString("dtype");
				String dataset = header.optString("dataset");

				refreshImage(img_data, dtype, shape, dataset);
				
			}
			catch(JSONException except)
			{
				IJ.error("Error receiving header: "+ except.getMessage());
				return;
			}
			
		}

		/**
		 * Displays the new image. Either updates an existing image with the new data, or creates one
		 * if either one does not exist, or if it is of the wrong size/data type.
		 * @param data    a buffer of the raw image data
		 * @param dtype   the data type of the image, as described by the header. Should be either uint8, uint16, or uint32
		 * @param shape   the dimensions of the image, as an array of [width, height]
		 * @param dataset the name of the dataset, if present.
		 */
		private void refreshImage(ByteBuffer data, String dtype, int[] shape, String dataset)
		{
			boolean need_new_processor = false;
			int bitdepth = dtype_map.get(dtype);
			ImageProcessor ip = img.getProcessor();
			Object img_pixels = null;
			LUT[] luts = img.getLuts();
			
			//image window may have been closed by user, which causes null pointer exceptions if trying to get the bit depth. 
			if(!img.isVisible())
			{
				need_new_processor = true;
			}
			txt_image_dims.setText(String.format("[%d, %d]", shape[0], shape[1])); //TODO: remove GUI changes from socket class
			txt_dataset.setText(dataset);
	
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
				//different bitdepths require different Image Processors for the data type, as well as different buffer types
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
					//copying data from the buffer is a little more involved as it requires casting from int to float. Fun times
					IntBuffer floatBuf = data.asIntBuffer();
					int[] tmp_array = new int[floatBuf.limit()];
					img_pixels = new float[floatBuf.limit()];
					floatBuf.get((int[])tmp_array);
					for(int i = 0; i < tmp_array.length; i++)
					{
						((float[])img_pixels)[i] = tmp_array[i];
					}
					break;

				default:
					IJ.error("Sorry, the datatype of this image is currently unsupported");
					return;

			}

			ip.setPixels(img_pixels);
			if(need_new_processor && luts.length != 0)
			{
				img.setLut(luts[0]);
				ip.resetMinAndMax();
			}
			img.updateAndDraw();
			img.updateStatusbarValue();
			if(!img.isVisible())
			{
				img.setTitle("Live View From: "+ socket_addr);
				img.show();
			}
			long time_taken = System.currentTimeMillis() - time_last_image;
			time_last_image = System.currentTimeMillis();
			image_count++;
		}

		/**
		 * Inverts the value that tells the socket wether to keep checking for new data on the socket.
		 * @return the inverted value. If it was <code>true</code> before calling this method, it's now <code>false</code>
		 * and vice versa
		 */
		public boolean invert_paused()
		{
			is_paused = !is_paused;
			return is_paused;
		}

	}//Class LiveViewSocket
}