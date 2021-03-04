//
// Created by gnx91527 on 14/01/19.
//

#ifndef ODINDATA_SEGFAULTHANDLER_H
#define ODINDATA_SEGFAULTHANDLER_H

#include <execinfo.h>

namespace OdinData {

    /**
     * Print out a stack trace
     *
     * This function will print out a stack trace.  It does not perform
     * name de-mangling to avoid any memory allocation deadlocking which
     * might suppress the printing of the trace.  The trace is output to
     * stderr by default to maximise the chance of the text making it to
     * a terminal when the application has ended with a fault.
     *
     * The resulting output can easily be de-mangled by copying and pasting
     * into
     *
     * www.demangler.com
     *
     * @param out the FILE to ouput the trace to (default stderr)
     * @param max_frames the maximum number of backtrace frames to print
     */
    void print_stack_trace(FILE *out = stderr, unsigned int max_frames = 63)
    {
        // Print a header
        fprintf(out, "stack trace:\n");

        // Declare storage array for stack trace address data
        void* addrlist[max_frames+1];

        // Retrieve current stack addresses
        uint32_t addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

        if (addrlen == 0){
            fprintf(out, "  \n");
            return;
        }

        // Dump the symbol trace to output.
        // backtrace_symbols_fd has been selected as it will not perform any malloc calls
        // The output will contain mangled symbols but these can be converted
        int fd = fileno(out);
        backtrace_symbols_fd(addrlist, addrlen, fd);
    }

    /**
     * Handle a caught signal.
     *
     * This funtcion will print a message to stderr to notifty which signal has been
     * caught and then initiate a stack trace dump.
     *
     * @param signum signal ID
     * @param si struct with signal information
     * @param unused
     */
    void abort_handler(int signum, siginfo_t* si, void* unused)
    {
        // Associate each signal with a signal name string.
        const char* name = NULL;
        switch(signum)
        {
            case SIGABRT: name = "SIGABRT";  break;
            case SIGSEGV: name = "SIGSEGV";  break;
            case SIGBUS:  name = "SIGBUS";   break;
            case SIGILL:  name = "SIGILL";   break;
            case SIGFPE:  name = "SIGFPE";   break;
            case SIGPIPE: name = "SIGPIPE";  break;
        }

        // Notify which signal was caught. Use fprintf as it is the
        // most basic output function. After a crash it is possible that more
        // complex output systems may be corrupted.
        if (name){
            fprintf(stderr, "Caught signal %d (%s)\n", signum, name);
        } else {
            fprintf(stderr, "Caught signal %d\n", signum);
        }

        // Dump a stack trace.
        print_stack_trace();

        // Now continue and quit the program.
        exit(signum);
    }

    /**
     * Initialise the segmentation fault handling for an application.
     *
     * This method will register a handler for the following signals:
     *
     * SIGABRT - Abort function has been called by another function
     * SIGSEGV - Invalid memory segment access
     * SIGBUS - Bus error
     * SIGILL - Illegal instruction
     * SIGFPE - Floating point exception
     * SIGPIPE - Write on a pipe with no reader, Broken pipe
     *
     * If any of these signals are caught after this function has been
     * called then the abort_handler method will be called.
     */
    void init_seg_fault_handler()
    {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = abort_handler;
        sigemptyset( &sa.sa_mask );

        sigaction( SIGABRT, &sa, NULL );
        sigaction( SIGSEGV, &sa, NULL );
        sigaction( SIGBUS,  &sa, NULL );
        sigaction( SIGILL,  &sa, NULL );
        sigaction( SIGFPE,  &sa, NULL );
        sigaction( SIGPIPE, &sa, NULL );
    }
}

#endif //ODINDATA_SEGFAULTHANDLER_H
