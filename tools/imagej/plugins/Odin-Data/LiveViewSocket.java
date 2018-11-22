import java.nio.*;
import java.util.HashMap;

import ij.IJ;
import ij.ImagePlus;
import ij.process.*;

import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMQ;
import org.zeromq.ZMQException;
import org.json.*;

/**
 * Class that contains the ZMQ subscriber socket and logic for what to do with an image when it arrives.
 * @author Adam Neaves
 * @version 0.1.0
 */
public class LiveViewSocket 
{
    private ZMQ.Socket socket;
    private ZMQ.Context context;    
    private ImagePlus img = null;
    private ImageFrame frame;   

    private String socket_addr;

    private boolean is_paused = false;

    private Thread zmqThread;

    private int image_count = 0;

    private static HashMap<String, Integer> dtype_map;

    /**
     * Constructor for the class. Sets up threads that run the ZMQ socket and background timer which counts the frames.
     * @param socket_addr    the address of the live view that the socket needs to subscribe to.
     * @throws ZMQException  if the socket address is invalid or malformed
     */
    public LiveViewSocket(String socket_addr) throws ZMQException
    {
        setup_socket(socket_addr);
        img = new ImagePlus();

        dtype_map = new HashMap<String, Integer>();
		dtype_map.put("uint8", 8);
		dtype_map.put("uint16", 16);
		dtype_map.put("uint32", 32);
		dtype_map.put("float", 33); //set to 33 so it can be parsed separately to the uint32

        zmqThread = new Thread(){
            @Override
            public void run()
            {
                run_socket();
            }
        };     

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
        //printMessage("Socket Shutdown Signal Received. Closing socket");
        zmqThread.interrupt();
        socket.close();
        context.close();
        img.hide();
    }

    /**
     * Sets up the socket, subscribing it to the supplied socket address
     * @param addr the socket address to subscribe to.
     * @throws ZMQException if the socket address supplied is malformed or otherwise invalid.
     */
    public void setup_socket(String addr) throws ZMQException
    {
        //printMessage("CREATING ZMQ SOCKET");
        context = ZMQ.context(1);
        socket = context.socket(ZMQ.SUB);
        try
        {
            socket.connect(addr);
            this.socket_addr = addr;
        }
        catch(ZMQException except)
        {
            //printMessage("EXCEPTION CONNECTING SOCKET");
            throw except;
            
        }
        
        socket.subscribe("".getBytes());
        //printMessage(String.format("Subscribed to addr %s",socket_addr));
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

            frame = new ImageFrame(dataset, dtype_map.get(dtype), shape);

            refreshImage(img_data, dtype, shape);
            
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
     */
    private void refreshImage(ByteBuffer data, String dtype, int[] shape)
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
        
        //need to check if the shape or the data type of the image have changed. If they have, we need to create a new Processor       
        if(!need_new_processor)
        {
            int current_bitdepth = ip.getBitDepth();
            int[] current_shape = new int[]{ip.getWidth(), ip.getHeight()};
            if(current_shape[0] != shape[0] || current_shape[1] != shape[1] || bitdepth != current_bitdepth)
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
                //copying data from the buffer is a little more involved as it requires casting from int to float.
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

    public boolean is_paused()
    {
        return is_paused;
    }

    public int get_image_count()
    {
        return image_count;
    }

    public void set_image_count(int val)
    {
        image_count = val;
    }

    public int[] get_shape()
    {
        if(frame != null)
            return frame.shape;
        else
        { 
            int[] val = {0,0};
            return val;
        }
    }

    public String get_dataset()
    {
        if(frame != null && frame.dataset != null)
            return frame.dataset;
        else return "None";
    }

    private class ImageFrame
    {
        private String dataset;
        private int bitdepth;
        private int[] shape;


        public ImageFrame(String dataset, int bitdepth, int[] shape)
        {
            this.dataset = dataset;
            this.bitdepth = bitdepth;
            this.shape = shape;
        }
    }
}//Class LiveViewSocket