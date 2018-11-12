import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;

class subscriber{

    private static Context context = ZMQ.context(1);
    private static ZMQ.Socket subscriber = context.socket(ZMQ.SUB);


    public static void run_socket()
    {
        while(!Thread.currentThread().isInterrupted())
        {
            ZMQ.Poller items = new ZMQ.Poller(1);
            items.register(subscriber, ZMQ.Poller.POLLIN);

            if(items.poll(0) == -1)
            {
                System.out.println("POLL RETURN -1");
                break;
            }

            if(items.pollin(0))
            {
                byte[] msg = subscriber.recv(ZMQ.NOBLOCK);

                String message = new String(msg);
                System.out.println("RECIVED MESSAGE:");
                System.out.println("'" + message.toString() + "'");
            }
        }
        
        System.out.println("THREAD INTERRUPTED");
        subscriber.close();
        context.term();
    }

    public static void main (String[] args) {
        
        // Context context = ZMQ.context(1);
        // ZMQ.Socket subscriber = context.socket(ZMQ.SUB);
        String socket_addr = "tcp://127.0.0.1:5020";
        if(args.length != 0)
        {
            socket_addr = args[0];
        }
        subscriber.connect(socket_addr);
        subscriber.subscribe("".getBytes());

        System.out.println(String.format("Subscribed to addr %s", socket_addr));

        Thread zmqThread = new Thread()
        {
            @Override
            public void run()
            {
                run_socket();
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