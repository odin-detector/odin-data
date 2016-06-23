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
                            help="set timeout for process completion")
        parser.add_argument('--fr_config', type=str, default='test_config/fr_test.config',
                            help="set configuration file for frameReceiver")
        parser.add_argument('--fp_config', type=str, default='test_config/fp_test.config',
                            help="set configuration file for frameProcessor")
        parser.add_argument('--use_processor', action='store_true',
                            help='use python frame processor instead of fileWriter')

        args = parser.parse_args()

        self.frames = args.frames
        self.interval = args.interval
        self.timeout = args.timeout
        self.fr_config = args.fr_config
        self.fp_config= args.fp_config
        self.use_processor = args.use_processor

    def run(self):

        rc = 0

        receiver = self.launch_process("bin/frameReceiver --config %s --logconfig test_config/fr_log4cxx.xml --debug 2 --frames %d" %
                                       (self.fr_config, self.frames))
        time.sleep(0.5)
        if self.use_processor:
            processor = self.launch_process("python -m frame_processor --config %s --bypass_mode --frames %d" %
                                            (self.fp_config, self.frames))
            writer = None
        else:
            writer = self.launch_process("bin/filewriter --logconfig=test_config/fw_log4cxx.xml --frames=%d --output=/tmp/integration_test.hdf5" %
                                         (self.frames))
            processor = None

        time.sleep(0.5)
        producer = self.launch_process("python -m frame_producer --destaddr 127.0.0.1:8000 --frames %d --interval %f" % (self.frames, self.interval))

        # Wait for producer to run to completion and validate output. Other processes should complete shortly thereafter, so use a short timeout
        rc = rc + self.validate_process_output(producer, "Producer", "%d frames completed" % self.frames, None)
        rc = rc + self.validate_process_output(receiver, "Receiver",
           "Specified number of frames \(%d\) received and released, terminating" % self.frames, self.timeout)
        if self.use_processor:
            rc = rc + self.validate_process_output(processor, "Processor",
               "Specified number of frames \(%d\) received, terminating" % self.frames, self.timeout)
        else:
            rc = rc + self.validate_process_output(writer, "FileWriter",
                'Specified number of frames \(%d\) received, terminating' % self.frames, self.timeout)

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

        if expected_in_output is not None:
            if not re.search(expected_in_output, process_stdout):
                if process_timedout:
                    timedout_msg = 'timed out and '
                else:
                    timedout_msg = ''
                print 'ERROR: {:s} process {:s}did not yield expected output (\"{:s}\")'.format(
                    name, timedout_msg, expected_in_output
                )
                print "Output follows:"
                print process_stdout
                print process_stderr
                rc = 1
            else:
                if process_timedout:
                    print 'WARNING: {:s} process timed out but produced expected output'.format(
                        name
                    )
                else:
                    print 'OK: {:s} process completed with expected output'.format(name)
        else:
            print 'OK: {:s} process completed (no validation of output)'.format(name)

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
