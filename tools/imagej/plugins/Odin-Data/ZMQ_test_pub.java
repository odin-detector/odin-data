import org.zeromq.ZMQ;
import org.zeromq.ZMQException;
import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;


class publisher{

    public static void run_socket(ZMQ.Socket socket, Context context)
    {
        while(!Thread.currentThread().isInterrupted())
        {
            String message1 = String.format("Sending message at %d", System.currentTimeMillis());
            System.out.println(message1);
            socket.send(message1, 0);
            try
            {
                Thread.sleep(1000);
            }
            catch(InterruptedException e)
            {
                System.out.println("Thread.sleep interrupted");
                break;
            }
            // String message2 = "Second part of multi part message";
            // socket.send(message2, 0);
            // try
            // {
            //     Thread.sleep(100);
            // }
            // catch(InterruptedException e)
            // {
            //     System.out.println("Thread.sleep interrupted");
            //     break;
            // }
        }

        System.out.println("THREAD INTERRUPTED");
        socket.close();
        context.term();
    }
    public static void main (String[] args) {
        Context context = ZMQ.context(1);
        ZMQ.Socket publisher = context.socket(ZMQ.PUB);
        String socket_addr = "tcp://127.0.0.1:5020";
        if(args.length != 0)
        {
            socket_addr = args[0];
        }
        publisher.bind(socket_addr);
        
        System.out.println(String.format("Subscribed to addr: %s", socket_addr));
        
        Thread zmqThread = new Thread()
        {
            @Override
            public void run()
            {
                run_socket(publisher, context);
            }
        };
        
        Runtime.getRuntime().addShutdownHook(new Thread(){
            @Override
            public void run(){
                System.out.println("W: Interrupt received, killing server…");
                
                try {
                    zmqThread.interrupt();
                    
                    zmqThread.join();
                } catch (Exception e) 
                {
                    System.out.println("INTERRUPT ERROR: " + e.getMessage());
                    e.printStackTrace();
                }
            }
        });

        zmqThread.start();
    }
}