#!/bin/env python

import argparse
import sys
import subprocess
import shlex
import time
from threading import Timer
import re

class IntegrationTest(object):
    
    def __init__(self):
 
        parser = argparse.ArgumentParser(prog="IntegrationTest", description="IntegrationTest - run PERCIVAL frame receiver integration test")
        
        parser.add_argument('--frames', '-n', type=int, default=100,
                            help='select number of frames to transmit in test')
        parser.add_argument('--interval', '-i', type=float, default=0.1,
                            help='set frame output interval in seconds')
        parser.add_argument('--timeout', '-t', type=float, default=5.0,
                            help="sets timeout for process completion")
    
        args = parser.parse_args()
    
        self.frames = args.frames
        self.interval = args.interval
        self.timeout = args.timeout
    
    def run(self):
        
        rc = 0
        
        receiver = self.launch_process("bin/frameReceiver --config test_config/fr_test.config --logconfig test_config/log4cxx.config --debug 2 --frames %d" % self.frames)
        time.sleep(0.5)
        processor = self.launch_process("python -m frame_processor --config test_config/fp_test.config --bypass_mode --frames %d" % self.frames)
        time.sleep(0.5)
        producer = self.launch_process("python -m frame_producer --destaddr 127.0.0.1:8000 --frames %d --interval %f" % (self.frames, self.interval))
        
        # Wait for producer to run to completion and validate output. Other processes should complete shortly thereafter, so use a short timeout
        rc = rc + self.validate_process_output(producer, "Producer", "%d frames completed" % self.frames, None)
        rc = rc + self.validate_process_output(receiver, "Receiver", 
           "Specified number of frames \(%d\) received and released, terminating" % self.frames, self.timeout)
        rc = rc + self.validate_process_output(processor, "Processor",
           "Specified number of frames \(%d\) received, terminating" % self.frames, self.timeout)
        
        if rc == 0:
            print "Integration test (%d frames) PASSED" % self.frames
        else:
            print "Integration test (%d frames) FAILED" % self.frames
            
        return rc
    
    def launch_process(self, cmd_line):
        
        proc = subprocess.Popen(shlex.split(cmd_line), stdout=subprocess.PIPE, stderr=subprocess.PIPE)        
        return proc
    
    def kill_process(self, process, timedout):
        
        timedout['value'] = True
        process.kill()
    
    def validate_process_output(self, process, name, expected_in_output=None, timeout=None):
        
        rc = 0
        
        (process_timedout, process_stdout, process_stderr) = self.wait_process_output(process, timeout)
        
        if process_timedout:
            print "ERROR:", name, "process timed out. Output follows:"
            print process_stdout
            print process_stderr
            rc = 1
                  
        elif expected_in_output is not None:
            if not re.search(expected_in_output, process_stdout):
                print "Failed to find expected output \"%s\" for %s process. Output follows:" % (expected_in_output, name)
                print process_stdout
                print process_stderr
                rc = 1
            else:
                print name, "process OK completed with expected output"
        else:
            print name, "process completed (no validation of output)"
            
        return rc
    
    def wait_process_output(self, process, timeout=None):
        
        timedout = { 'value' : False }
        
        if timeout is not None:
            timer = Timer(timeout, self.kill_process, [process, timedout])
            timer.start()
            
        (process_stdout, process_stderr) = process.communicate()
        
        if timeout is not None:
            timer.cancel()

        return (timedout['value'], process_stdout, process_stderr)
    
if __name__ == '__main__':
    
    rc= IntegrationTest().run()
    sys.exit(rc)