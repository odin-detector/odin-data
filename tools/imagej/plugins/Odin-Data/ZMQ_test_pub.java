import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;


class publisher{
    public static void main (String[] args) {
        Context context = ZMQ.context(1);
        ZMQ.Socket publisher = context.socket(ZMQ.PUB);
        String socket_addr = "tcp://127.0.0.1:5020";
        if(args.length != 0)
        {
            socket_addr = args[0];
        }
        publisher.bind(socket_addr);

        while(true)
        {
            String message = String.format("Sending message at %d", System.currentTimeMillis());
            System.out.println(message);
            publisher.send(message, 0);
            try
            {
                Thread.sleep(10);
            }
            catch(InterruptedException e)
            {
                System.out.println("Thread.sleep interrupted");
            }
        }

        
    }
}