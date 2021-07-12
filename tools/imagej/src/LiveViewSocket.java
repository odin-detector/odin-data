 import java.nio.*;
import java.util.Date;
import java.util.HashMap;
import java.util.Observable;

import ij.IJ;
import ij.ImagePlus;
import ij.process.*;

import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMQ;
import org.zeromq.ZMQException;
import org.json.*;

/**
 * Class that contains the ZMQ subscriber socket and logic for what to do with an image when it arrives.
 * @author Ashley Neaves
 * @version 0.1.0
 */
public class LiveViewSocket 
{
    private ZMQ.Socket socket;
    private ZMQ.Context context;    
    private ImagePlus img = null;
    private static ImageFrame frame;   
    private Live_View parent; 

    private String socket_addr;

    private boolean is_paused = false;

    private Thread zmqThread;

    private static HashMap<String, Integer> dtype_map;

    /**
     * Constructor for the class. Sets up threads that run the ZMQ socket and background timer which counts the frames.
     * @param socket_addr    the address of the live view that the socket needs to subscribe to.
     * @param parent         The Live View class that created this instance. Used to print messages on the GUI.
     * @throws ZMQException  if the socket address is invalid or malformed
     */
    public LiveViewSocket(String socket_addr, Live_View parent) throws ZMQException
    {
        setupSocket(socket_addr);
        img = new ImagePlus();
        frame = new ImageFrame();

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
        this.parent = parent;
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
    }

    /**
     * Closes down the socket and context, allowing the plugin to close cleanly.
     * Interrupts the thread running the socket.
     */
    public void shutdownSocket()
    {
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
    public void setupSocket(String addr) throws ZMQException
    {
        context = ZMQ.context(1);
        socket = context.socket(ZMQ.SUB);
        try
        {
            socket.connect(addr);
            this.socket_addr = addr;
        }
        catch(ZMQException except)
        {
            throw except;
            
        }
        
        socket.subscribe("".getBytes());
        return;
    }

    /**
     * Receives a frame from the ZMQ socket. The frame consists of a JSON header message,
     * and then a message containing a blob of binary data for the image itself.
     */
    private void recvFrame()
    {
        //Get header from first ZMQ message, turn it into a JSON Object
        try
        {
            JSONObject header = new JSONObject(new String(socket.recv()));
            ByteBuffer img_data = ByteBuffer.wrap(socket.recv());
            JSONArray JSONshape = header.optJSONArray("shape");
            int[] shape = new int[]{JSONshape.getInt(1), JSONshape.getInt(0)};

            String dtype = header.optString("dtype");
            String dataset = header.optString("dataset");

            frame.setDataset(dataset); 
            frame.setBitdepth(dtype_map.get(dtype));
            frame.setShape(shape);
            frame.setTimestamp(new Date());

            refreshImage(img_data, dtype, shape);
            
        }
        catch(JSONException except)
        {
            parent.printMessage("ERROR: Header Received was not formatted as expected.");
            return;
        }
        
    }

    /**
     * Works out if the current Image Processor needs to be replaced due to a change in bitdepth, size, or the closing of the window
     * @param shape    the shape of the new image
     * @param bitdepth the bitdepth of the new image
     * @return false, if the shape and bitdepth are the same, true if anything has changed.
     */
    private boolean needsNewProcessor(int[] shape, int bitdepth)
    {
        boolean need_new_processor = false;
        ImageProcessor ip = img.getProcessor();

        if(!img.isVisible())
        {
            need_new_processor = true;
        }
        else
        {   //this block must be in an else, because "ip" is null if the img is not visible,
            //and trying to access it causes null pointer issues
            int current_bitdepth = ip.getBitDepth();
            int[] current_shape = new int[]{ip.getWidth(), ip.getHeight()};
            if(current_shape[0] != shape[0] || current_shape[1] != shape[1] || bitdepth != current_bitdepth)
            {
                need_new_processor = true;
            }
        }

        return need_new_processor;
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
        int bitdepth = dtype_map.get(dtype);
        ImageProcessor ip = img.getProcessor();
        Object img_pixels = null;
        LUT[] luts = img.getLuts();

        boolean need_new_processor = needsNewProcessor(shape, bitdepth);

        int expected_image_size = shape[0]*shape[1]*bitdepth/8; //image size in bytes
        if(expected_image_size != data.limit())
        {
            parent.printMessage("ERROR: Image size seems wrong. Please ensure the image matches the header.");
            return;
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
            case 33: //For floats. Automatically goes to default for now
            default:
                parent.printMessage("ERROR: the datatype of this image is currently unsupported");
                return;

        }
        try{
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
        }
        catch(ArrayIndexOutOfBoundsException except)
        {
            parent.printMessage("ERROR: Image recieved seems incorrect. Ensure it matches the header sent.");
            return; 
        }
    }

    /**
     * Creates a copy of the image window, displaying it alongside the live image
     * @return <code>True</code> if the image is successfully duplicated, else <code>False</code>
     */
    public boolean makeImageCopy()
    {
        ImageProcessor ip = img.getProcessor();
        if(ip != null)
        {
            ImagePlus img_copy = img.duplicate();
            img_copy.setTitle(String.format("Snapshot of Image at %s", frame.getTimestamp().toString()));
            img_copy.show();
            return true;
        }
        else return false;
    }

    /**
     * Inverts the value that tells the socket whether to keep checking for new data on the socket.
     * @return the inverted value. If it was <code>true</code> before calling this method, it's now <code>false</code>
     * and vice versa
     */
    public boolean invertPaused()
    {
        is_paused = !is_paused;
        
        return is_paused;
    }

    /**
     * 
     * @return If the socket is currently paused.
     */
    public boolean isPaused()
    {
        return is_paused;
    }

    /**
     * 
     * @return the current Image Frame.
     */
    public ImageFrame getImageFrame()
    {
        return frame;
    }


    /**
     * This class holds the metadata of the current image. It can be "Observed",
     * meaning it can automatically inform other parts of the program if any of its
     * values change.
     */
    public class ImageFrame extends Observable
    {
        private String dataset;
        private int bitdepth;
        private int[] shape;
        private Date timestamp;

        /**
         * Constructor that sets the inital values for the image metadata.
         * @param dataset  the dataset the image came from. Not always used, depending on the detector
         * @param bitdepth The bitdepth of the image. Usually either 8, 16, or 32
         * @param shape    The Dimensions of the data in pixels, as an array of two values: <code>[width, height]</code>
         */
        public ImageFrame(String dataset, int bitdepth, int[] shape)
        {
            this.dataset = dataset;
            this.bitdepth = bitdepth;
            this.shape = shape;
            this.timestamp = null;
        }

        /**
         * Generic Constructor, which sets some default values for the metadata.
         * @see ImageFrame#ImageFrame(String, int, int[])
         */
        public ImageFrame()
        {
            this("None", 0, new int[]{0,0});
        }

        /**
         * Sets the dataset of the image. If this changes, it will notify observers of the change.
         * @param dataset the new name of the dataset.
         */
        public void setDataset(String dataset)
        {
            if(!dataset.equals(this.dataset))
            {
                this.dataset = dataset;
                setChanged();
                notifyObservers("dataset");
            }
        }

        /**
         * Sets the bitdepth of the image. If this changes, it will notify observers of the change.
         * @param bitdepth the new bitdepth. Should be either 8, 16 or 32.
         */
        public void setBitdepth(int bitdepth)
        {
            if(this.bitdepth != bitdepth)
            {
                this.bitdepth = bitdepth;
                setChanged();
                notifyObservers("bitdepth");
            }
        }

        /**
         * Sets the new dimensions of the image. If this changes, it will notify observers of the change.
         * @param shape The new dimensions, measured in pixels, as an array: <code>[width, height]</code>
         */
        public void setShape(int[] shape)
        {
            if(shape[0] != this.shape[0] || shape[1] != this.shape[1])
            {
                this.shape[0] = shape[0];
                this.shape[1] = shape[1];
                setChanged();
                notifyObservers("shape");
            }
        }

        /**
         * Sets the timestamp of the current image, representing the time it was read from the socket.
         * @param timestamp the time the image was read from the socket. 
         * As this should always be a new value, it always alerts observers of the change.
         */
        public void setTimestamp(Date timestamp)
        {
            this.timestamp = timestamp;
            setChanged();
            notifyObservers("timestamp");
        }

        /**@return the current dataset of the image */
        public String getDataset()
        {
            return dataset;
        }
        /**@return the current bitdepth of the image */
        public int getBitDepth()
        {
            return bitdepth;
        }
        /**@return the dimensions of the image in an array */
        public int[] getShape()
        {
            return shape;
        }
        /**@return the time and date the image was created from the socket */
        public Date getTimestamp()
        {
            return timestamp;
        }
    }
}//Class LiveViewSocket