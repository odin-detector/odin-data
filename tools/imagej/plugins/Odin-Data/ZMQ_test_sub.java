import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;

class subscriber{
    public static void main (String[] args) {

        
        Context context = ZMQ.context(1);
        ZMQ.Socket subscriber = context.socket(ZMQ.SUB);
        String socket_addr = "tcp://127.0.0.1:5020";
        if(args.length != 0)
        {
            socket_addr = args[0];
        }
        subscriber.connect(socket_addr);
        subscriber.subscribe("".getBytes());

        System.out.println(String.format("Subscribed to addr %s", socket_addr));

        while(true)
        {
            String message = new String(subscriber.recv(0));
            System.out.println("RECIVED MESSAGE:");
            System.out.println("'" + message.toString() + "'");
        }

        
    }
}