// A test application to receive the message published in FrameProcessorTest
#include <assert.h>
#include <stdio.h>

#include <zmq.h>

int main(int argc, char** argv)
{
    void* context = zmq_ctx_new();
    void* subscriber = zmq_socket(context, ZMQ_SUB);
    int rc = zmq_connect(subscriber, argv[1]);
    assert(rc == 0);
    rc = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
    assert(rc == 0);

    printf("Listening on %s\n", argv[1]);

    // Receive one message
    int LENGTH = 100;
    char message[LENGTH];
    rc = zmq_recv(subscriber, message, LENGTH, 0);
    assert(rc != -1);
    printf("Received '%s'\n", message);

    zmq_close(subscriber);
    zmq_ctx_destroy(context);

    return 0;
}
