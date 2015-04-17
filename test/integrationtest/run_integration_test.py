#!/bin/env python

import sys
import subprocess
import shlex
import time
from threading import Timer
import re

class IntegrationTest(object):
    
    def __init__(self):
        
        self.num_frames = 100
    
    def run(self):
        
        rc = 0
        
        receiver = self.launch_process("bin/frameReceiver --config test_config/fr_test.config --logconfig test_config/log4cxx.config --debug 2 --frames %d" % self.num_frames)
        time.sleep(0.5)
        processor = self.launch_process("python -m frame_processor --config test_config/fp_test.config --bypass_mode --frames %d" % self.num_frames)
        time.sleep(0.5)
        producer = self.launch_process("python -m frame_producer --destaddr 127.0.0.1:8000 --frames %d" % self.num_frames)
        
        rc = rc + self.validate_process_output(producer, "Producer", "%d frames completed" % self.num_frames)
        rc = rc + self.validate_process_output(receiver, "Receiver", "Specified number of frames \(%d\) received and released, terminating" % self.num_frames)
        rc = rc + self.validate_process_output(processor, "Processor", "Specified number of frames \(%d\) received, terminating" % self.num_frames)
        
        if rc == 0:
            print "Integration test (%d frames) PASSED" % self.num_frames
        else:
            print "Integration test (%d frames) FAILED" % self.num_frames
            
        return rc
    
    def launch_process(self, cmd_line):
        
        proc = subprocess.Popen(shlex.split(cmd_line), stdout=subprocess.PIPE, stderr=subprocess.PIPE)        
        return proc
    
    def kill_process(self, process, timedout):
        
        timedout['value'] = True
        process.kill()
    
    def validate_process_output(self, process, name, expected_in_output=None):
        
        rc = 0
        
        (process_timedout, process_stdout, process_stderr) = self.wait_process_output(process)
        
        if process_timedout:
            print "ERROR:", name, "process timed out. Output follows:"
            print process_stdout
            print process_stderr
            rc = 1
                  
        if expected_in_output is not None:
            if not re.search(expected_in_output, process_stdout):
                print "Failed to find expected output \"%s\" for %s process", (expected_in_output, name)
                rc = 1
                 
        return rc
    
    def wait_process_output(self, process, timeout = 5.0):
        
        timedout = { 'value' : False }
        timer = Timer(timeout, self.kill_process, [process, timedout])
        
        timer.start()
        (process_stdout, process_stderr) = process.communicate()
        timer.cancel()

        return (timedout['value'], process_stdout, process_stderr)
    
if __name__ == '__main__':
    
    rc= IntegrationTest().run()
    sys.exit(rc)