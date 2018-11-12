import java.awt.*;
import ij.*;
import ij.gui.*;
import ij.process.*;
import ij.plugin.PlugIn;

import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMQ;


/** This a prototype ImageJ plugin. */
public class Live_View implements PlugIn{

	public void run(String arg) 
	{
		long start = System.currentTimeMillis();
		int w = 400, h = 400;
		ImageProcessor ip = new ShortProcessor(w, h);
		short[] pixels = (short[])ip.getPixels();
		int i = 0;
		for (int y = 0; y < h; y++) {
			short red = (short)((y * 255) / (h - 1));
			for (int x = 0; x < w; x++) {
				
				pixels[i++] = red;
			}
		}
		new ImagePlus("Live View", ip).show();
		PrintMessage("Time Taken: "+(System.currentTimeMillis()-start));

		setup_socket("tcp://127.0.0.1:2020");
	}

	public void run_socket(Socket socket, Context context)
	{
		while(!Thread.currentThread().isInterrupted())
        {
            ZMQ.Poller items = new ZMQ.Poller(1);
            items.register(socket, ZMQ.Poller.POLLIN);

            if(items.poll(0) == -1)
            {
                PrintMessage("POLL RETURN -1");
                break;
            }

            if(items.pollin(0))
            {
                byte[] msg = socket.recv(ZMQ.NOBLOCK);

                String message = new String(msg);
                PrintMessage("RECIVED MESSAGE: '" + message + "'");
            }
        }
        
        PrintMessage("THREAD INTERRUPTED");
        socket.close();
        context.term();
	}

	public void setup_socket(String socket_addr)
	{
		PrintMessage("CREATING ZMQ SOCKET");
		Context context = ZMQ.context(1);
		Socket subscriber = context.socket(ZMQ.SUB);
		subscriber.connect(socket_addr);
		subscriber.subscribe("".getBytes());
		PrintMessage(String.format("Subscribed to addr %s",socket_addr));

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
                PrintMessage("Interrupt received, killing server…");
                
                try {
                    zmqThread.interrupt();
                    
                    zmqThread.join();
                } catch (Exception e) 
                {
                    PrintMessage("INTERRUPT ERROR: " + e.getMessage());
                    e.printStackTrace();
                }
            }
        });

        zmqThread.start();
	}

	public void PrintMessage(String message)
	{
		System.out.println(message);
		IJ.log(message);
	}
}