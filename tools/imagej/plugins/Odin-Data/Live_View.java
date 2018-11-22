import java.awt.*;
import java.awt.event.*;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import ij.plugin.frame.*;
import ij.*;
import ij.process.*;
import ij.gui.*;

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

	LiveViewSocket socket = null;

	boolean is_logging = false;

	private TimerTask status_update;
    private Timer frame_counter;

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
				frame_counter.cancel();
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
					frame_counter.cancel();
				} 
				catch (Exception e) 
				{
					printMessage("INTERRUPT ERROR: " + e.getMessage());
				}
			}
		});


        status_update = new TimerTask(){
            long prev_time = System.currentTimeMillis();
            @Override
            public void run() {
                if(socket != null && !socket.is_paused())
				{
					int image_count = socket.get_image_count();
					socket.set_image_count(0);
                    long start_time = System.currentTimeMillis();
                    float elapsed_time = (start_time - prev_time)/1000f;
                    float fps = image_count / elapsed_time;
                    
					txt_fps.setText(String.format("%.2f fps", fps));
					txt_dataset.setText(socket.get_dataset());
					int[] shape = socket.get_shape();
					txt_image_dims.setText(String.format("[%d, %d]", shape[0], shape[1]));
                    printMessage(String.format("Received %d images in %.3f seconds", image_count, elapsed_time));
                    prev_time = start_time;
					
					
                }
            }
        };
        frame_counter = new Timer(true);
        int timer_delay = 2000;
        frame_counter.schedule(status_update, 0, timer_delay);
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
					printMessage(String.format("Socket Subscribed to address: %s", socket_addr));
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

}