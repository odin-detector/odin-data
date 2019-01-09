import java.awt.*;
import java.awt.event.*;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.LinkedList;
import java.util.Observer;
import java.util.Queue;
import java.util.Observable;
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
public class Live_View extends PlugInFrame implements ActionListener, Observer
{
    private Panel panel;
    TextField txt_socket_addr;
    TextField txt_image_dims;
    TextField txt_dataset;
    TextField txt_status;
    TextField txt_fps;

    Label lbl_is_connected;

    Button btn_live_on_off;
    Button btn_connect;
    Button btn_snapshot;
    Checkbox chk_logging;

    private static Frame instance;
    
    private String socket_addr = "tcp://127.0.0.1:5020";

    LiveViewSocket socket = null;
    LiveViewSocket.ImageFrame image_frame;

    long last_image_time = new Date().getTime();
    Queue<Long> time_avg_queue;
    long total = 0;
    int avg_size = 10; //size of the queue used for the rolling average of time between frames

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
        time_avg_queue = new LinkedList<>();

        addKeyListener(IJ.getInstance());
        setLayout(new FlowLayout());
        panel = buildGUI();
        add(panel);
        
        pack();
        GUI.center(this);

        //add listener to window close events to shutdown socket
        addWindowListener(new WindowAdapter() 
        {
            public void windowClosing(WindowEvent e)
            {
                printMessage("Window Closing");
                if(socket != null)
                {
                    socket.shutdownSocket();
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
                    socket.shutdownSocket();
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
                if(socket != null)
                {
                    long start_time = System.currentTimeMillis();
                    Date image_timestamp = image_frame.getTimestamp();
                    if(image_timestamp != null)
                    {
                        long time_of_frame = image_timestamp.getTime();
                        printMessage(String.format("Time since last frame was received: %.3f seconds", (start_time-time_of_frame)/1000f));
                    }
                    else
                    {
                        printMessage("No Images recieved yet.");
                    }
                }
            }
        };
        frame_counter = new Timer(true);
        int timer_delay = 5000;
        frame_counter.schedule(status_update, 0, timer_delay);
    }

    @Override
    public void update(Observable o, Object arg)
    {
        switch(arg.toString().toLowerCase())
        {
            case "timestamp":
                long new_time = image_frame.getTimestamp().getTime(); //get timestamp
                long time_diff = new_time - last_image_time; //get time between recieiving frames
                last_image_time = new_time;
                total += time_diff;
                time_avg_queue.add(time_diff);
                if(time_avg_queue.size() > avg_size)
                {
                    total -= time_avg_queue.remove();
                }
                printMessage(String.format("Time since last frame was received: %.3f seconds", time_diff/1000f));
                txt_fps.setText(String.format("%.3f/s", 1000f/(total/time_avg_queue.size())));
                break;
            case "dataset":
                txt_dataset.setText(image_frame.getDataset());
                break;
            case "shape":
                int[] new_shape = image_frame.getShape();
                txt_image_dims.setText(String.format("[%d, %d]", new_shape[0], new_shape[1]));
                break;
            case "bitdepth":
            default:
                break;
        }
    }

    @Override
    public void run(String args)
    {
        if("help".equalsIgnoreCase(args))
        {
            String help_text;
            help_text = new String("Live View Plugin designed to pair with the Odin Data Live View Plugin.\n"+
                                   "Subscribes to the supplied ZMQ socket address, and receives header and image data\n"+
                                   " and displays it in an ImageJ Window.");

            IJ.showMessage("Odin live View Help", help_text);            
        }
        else
        {
            if(instance ==  null)
            {
                this.setVisible(true);
            }
            else
            {
                instance.setVisible(true);
            }
        }
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
        lbl_is_connected = new Label("DISCONNECTED");

        txt_socket_addr = new TextField(socket_addr, 25);
        txt_socket_addr.setEditable(true);
        txt_socket_addr.setBackground(Color.red);
        txt_socket_addr.setForeground(Color.white);
        txt_image_dims = new TextField("[0 , 0]", 12);
        txt_image_dims.setEditable(false);
        txt_dataset = new TextField("None", 12);
        txt_dataset.setEditable(false);
        txt_status = new TextField("", 70);
        txt_status.setEditable(false);
        txt_fps = new TextField("", 8);
        txt_fps.setEditable(false);
        
        btn_live_on_off = new Button("  PAUSE  ");
        btn_connect = new Button(" CONNECT  ");
        btn_snapshot = new Button("SNAP");
        chk_logging = new Checkbox();
        chk_logging.setState(is_logging);
        
        //constraints for components.
        GridBagConstraints constraints = new GridBagConstraints();
        constraints.insets = new Insets(2,2,2,2);
        constraints.anchor = GridBagConstraints.CENTER;
        int x = 0; //horizontal grid position
        constraints.gridwidth = 2;
        addComponent(txt_socket_addr, lbl_socket_addr, panel, x++, constraints);
        constraints.gridwidth = 1;
        x++;
        addComponent(btn_connect, null,                panel, x++, constraints);
        addComponent(btn_live_on_off, null,            panel, x++, constraints);
        addComponent(txt_dataset, lbl_dataset,         panel, x++, constraints);
        addComponent(txt_image_dims, lbl_image_dims,   panel, x++, constraints);
        addComponent(txt_fps, lbl_fps,                 panel, x++, constraints);
        addComponent(btn_snapshot, null,               panel, x++, constraints);
        addComponent(chk_logging, lbl_logging,         panel, x++, constraints);

        //bottom row
        addComponent(lbl_is_connected, panel, 0, 2, constraints);
        constraints.anchor = GridBagConstraints.EAST;
        addComponent(lbl_status, panel, 1, 2, constraints);
        constraints.anchor = GridBagConstraints.WEST;
        constraints.gridwidth = 6;        
        addComponent(txt_status, panel, 2, 2, constraints);

        //attatching event listeners
        btn_live_on_off.addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent event)
            {
                if(socket != null){
                    boolean is_paused = socket.invertPaused();        
                    if(!is_paused)
                    {
                        btn_live_on_off.setLabel("PAUSE");
                    }
                    else
                    {
                        btn_live_on_off.setLabel("UNPAUSE");
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
                    activateSocket();
                }
                else
                {
                    deactivateSocket();
                }
            }
        });

        btn_snapshot.addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent event)
            {
                if(socket != null)
                {
                    boolean duplicate = socket.makeImageCopy();
                    if(!duplicate)
                    {
                        printMessage("COPY FAILED");
                    }
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
     * Add a component and it's matching label to the supplied panel. Adds the label to the top row, and the
     * component below the label in the same column.
     * @param comp the component to be added
     * @param label the label of the component. This can be null for no label.
     * @param panel the panel that the components need to be added to
     * @param x the horizontal grid poisiton of the components. Starts at 0 for far left.
     * @param constraints the grid bag constraints that provide other constraints, such as margins and anchor.
     */
    private void addComponent(Component comp, Component label, Panel panel, int x, GridBagConstraints constraints)
    {
        constraints.gridx = x;
        if(label != null)
        {
            constraints.gridy = 0;
            panel.add(label, constraints);
        }
        constraints.gridy = 1;
        panel.add(comp, constraints);
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

    @Override
    public void actionPerformed(ActionEvent e)
    {
        printMessage("Action Performed: " + e.toString());
    }

    /**
     * Creates the Live View socket and collects the required references from the newly created socket.
     * If the socket cannot connect, the method will fail and a IJ Error window displayed.
     */
    private void activateSocket()
    {
        try
        {
        socket = new LiveViewSocket(socket_addr, this);
        image_frame = socket.getImageFrame();
        image_frame.addObserver(this);
        btn_connect.setLabel("DISCONNECT");
        txt_socket_addr.setBackground(Color.green);
        txt_socket_addr.setForeground(Color.black);
        txt_socket_addr.setEditable(false);
        lbl_is_connected.setText("CONNECTED");
        printMessage(String.format("Socket Subscribed to address: %s", socket_addr));
        }
        catch(ZMQException except)
        {
            IJ.error("ERROR CONNECTING SOCKET", "Check if the socket address is valid.");
        }
    }

    /**
     * Closes the Live View Socket and removes any references to any part of it so it can be collected by Java's garbage collection.
     */
    private void deactivateSocket()
    {
        socket.shutdownSocket();
        image_frame.deleteObservers();
        socket = null;
        btn_connect.setLabel("CONNECT");
        txt_socket_addr.setEditable(true);
        txt_socket_addr.setBackground(Color.red);
        txt_socket_addr.setForeground(Color.white);
        lbl_is_connected.setText("DISCONNECTED");
    }

}