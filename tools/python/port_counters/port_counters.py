#!/bin/env python

import argparse
import json
import sys
import subprocess
import re
from datetime import datetime
from pysnmp.entity.rfc3413.oneliner import cmdgen
from pysnmp.error import PySnmpError

class PortCounters(object):
        
    def __init__(self):

        # Define a default switch name to access
        default_switch_name = 'devswitch5920'
        
        # Define a list of nodes to access. listing interfaces and associated switch ports
        self.node_list = { 
            'te7aegnode01' : {'iface' : ['p2p1', 'p2p2'], 'port' : [1,  9] },
            'te7aegnode02' : {'iface' : ['p2p1', 'p2p2'], 'port' : [2, 10] },
            'te7aegnode03' : {'iface' : ['em1',  'em2' ], 'port' : [3, 11] },
            'te7aegnode04' : {'iface' : ['em1',  'em2' ], 'port' : [4, 12] },
            'te7aegnode05' : {'iface' : ['em1',  'em2' ], 'port' : [5, 13] },
            'te7aegnode06' : {'iface' : ['em1',  'em2' ], 'port' : [6, 14] },
            'te7aegnode07' : {'iface' : ['em1',  'em2' ], 'port' : [7, 15] },
            'te7aegnode08' : {'iface' : ['em1',  'em2' ], 'port' : [8, 16] },
        }
        
        parser = argparse.ArgumentParser(prog="port_counters", 
                                         description="port_counters - capture port packet counters from a switch via SNMP and calculate deltas",
                                         formatter_class=argparse.ArgumentDefaultsHelpFormatter)

        parser.add_argument('--addr', '-a', dest='switch_addr', type=str, default=default_switch_name, 
                            help='Address of switch to access')
        parser.add_argument('start_file', nargs='?',
                            help='JSON-formatted file containing starting switch counters for calculate mode')
        parser.add_argument('end_file', nargs='?',
                            help='JSON-formatted file containing starting switch counters for calculate mode')        
        args = parser.parse_args()
        
        self.switch_addr = args.switch_addr
        self.start_file  = args.start_file
        self.end_file    = args.end_file
                
        if self.start_file != None and self.end_file != None:
            self.calculate_deltas = True
        else:
            self.calculate_deltas = False

        # Dict of SNMP OIDs to retrieve from device
        self.switch_oids = {
            'ifIndex'       : (1,3,6,1,2,1,2,2,1,1),
            'ifName'        : (1,3,6,1,2,1,31,1,1,1,1),
            'ifAlias'       : (1,3,6,1,2,1,31,1,1,1,18),
            'ifHCInOctets'  : (1,3,6,1,2,1,31,1,1,1,6),
            'ifHCOutOctets' : (1,3,6,1,2,1,31,1,1,1,10),
            'ifInUcastPkts' : (1,3,6,1,2,1,2,2,1,11),
            'ifOutUcastPkts' : (1,3,6,1,2,1,2,2,1,17),
        }
        
        # Limit to restrict port stats to physical ports only
        self.maxPhysicalPort = 25
            
    def run(self):
        
        if self.calculate_deltas:
            self.calculate_port_deltas(self.start_file, self.end_file)
        else:
            self.save_port_counters()
            
    def save_port_counters(self):

        port_counters = {}
        port_counters['switch_counters'] = self.get_switch_counters()
        port_counters['node_counters'] = self.get_node_counters()
                
        file_name = datetime.now().strftime("portCounters-%Y%m%d-%H%M%S.json")

        print("Saving port counters to file {}".format(file_name))
                    
        with open(file_name, 'w') as f:
            json.dump(port_counters, f, sort_keys=True, indent=4, separators=(',', ':'))

    
    def get_switch_counters(self):

        switch_counters = {}
                
        cmd_gen = cmdgen.CommandGenerator()

        print("Retrieving port counters for switch {} ...".format(self.switch_addr))
        try:        
            error_indication, error_status, error_index, var_binds = cmd_gen.nextCmd(
                cmdgen.CommunityData('server', 'public', 1),
                cmdgen.UdpTransportTarget((self.switch_addr, 161)),
                self.switch_oids['ifName'],
                self.switch_oids['ifIndex'],
                self.switch_oids['ifInUcastPkts'],
                self.switch_oids['ifOutUcastPkts']
            )
        except PySnmpError as e:
            print("Error retrieving port counters from switch: {}".format(e))
        else:
            
            # Check for errors and report, otherwise write port counters to output file
            if error_indication:
                print("SNMP engine error: {}", format(error_indication))
            else:
                if error_status:
                    print('SNMP error: {} at {}'.format(
                        error_status.prettyPrint(), error_index and var_binds[int(error_index)-1] or '?'))
                else:                
                    for line in var_binds:
                        idx = var_binds.index(line)
                        name = str(var_binds[idx][0][1])
                        if name != '':
                            index = int(str(var_binds[idx][1][1]))
                            in_pkts = int(str(var_binds[idx][2][1]))
                            out_pkts = int(str(var_binds[idx][3][1]))
                            if index <= self.maxPhysicalPort:
                                switch_counters[index] = (name, in_pkts, out_pkts)
                                #print index, name, in_pkts, out_pkts
        
        return switch_counters
    
    def get_node_counters(self):

        node_counters={}

        rxline_re = re.compile('RX\ packets')
        txline_re = re.compile('TX\ packets')
        counter_re = re.compile('\d+')
        
        for node in sorted(self.node_list):
            for interface in self.node_list[node]['iface']:
                cmd_str = ["ssh", node, "/sbin/ifconfig", interface]
    #            print cmd_str
                print("Retrieving packet counters for node {} interface {} ...".format(node, interface))
                response = subprocess.Popen(cmd_str, stdout=subprocess.PIPE).communicate()[0]
    #            print response
                for line in str.splitlines(response):
    #                print ">>>", line
                    if rxline_re.search(line):
                        rx_counters = [int(x) for x in counter_re.findall(line)]
                    if txline_re.search(line):
                        tx_counters = [int(x) for x in counter_re.findall(line)]
                        
    #            print rx_counters, tx_counters
                node_iface_key = node + "_" + interface
                node_counters[node_iface_key] = [rx_counters, tx_counters]
                
    #    print node_counters
        
        return node_counters
        
    def calculate_port_deltas(self, start_file, end_file):
        
        print("Reading start counters from file {}".format(start_file))
        with open(start_file, 'r') as f1:
            port_counters_start = json.load(f1)
        
        print("Reading end counters from file {}".format(end_file))
        with open(end_file, 'r') as f2:
            port_counters_end = json.load(f2)
                
        # Build a reverse mapping of switch port numbers to nodes/interfaces
        port_mapping = {}
        for node in self.node_list:
            for interface, port in zip(self.node_list[node]['iface'], self.node_list[node]['port']):
                port_mapping[port] = node + "_" + interface
        
        port_deltas={}
        
        port_deltas['switch'] = self.calculate_switch_deltas(
            port_counters_start['switch_counters'], port_counters_end['switch_counters'])
        port_deltas['node'] = self.calculate_node_deltas(
            port_counters_start['node_counters'], port_counters_end['node_counters'])

        print("")
        print("Switch Port                                         : Node Port")
        print("{}".format(''.join(['=']*209)))
        print("{:3s}{:26s} {:>10s} {:>10s} : {:20}  :  {} {}".format(
                "Idx", " Name", "InPkts", "OutPkts", "Node",
                " ".join("{0:>12}".format(rx_key) for rx_key in ("RX Packets", "RX Errors", "RX Dropped", "RX Overrun", "RX Frame")),
                " ".join("{0:>12}".format(tx_key) for tx_key in ("TX Packets", "TX Errors", "TX Dropped", "TX Overrun", "TX Carrier"))               
             ))
        
        for index in sorted(port_deltas['switch'], key=lambda k: int(k)):
            (name, in_delta, out_delta) = port_deltas['switch'][index]
            if int(index) in port_mapping:
                node_iface = port_mapping[int(index)]
                (rx_delta, tx_delta) = port_deltas['node'][node_iface]
                node_str = "{:20}  :  {} {}".format(
                    node_iface,
                    " ".join("{0:12d}".format(rx_i) for rx_i in rx_delta),
                    " ".join("{0:12d}".format(tx_i) for tx_i in tx_delta)
                )
            else:
                node_str = ""
            print("{:02d} {:>26s} {:10d} {:10d} : {:s}".format(int(index), name, in_delta, out_delta, node_str))
        
                            
    def calculate_switch_deltas(self, counters_start, counters_end):
       
        switch_deltas = {}
        
        for index in sorted(counters_start, key=lambda k: int(k)):
            name = counters_start[index][0]
            in_delta  = counters_end[index][1] - counters_start[index][1]
            out_delta = counters_end[index][2] - counters_start[index][2]
            switch_deltas[index] = (name, in_delta, out_delta)
            
        return switch_deltas

    def calculate_node_deltas(self, counters_start, counters_end):
        
        node_deltas = {}
          
        for node_iface in sorted(counters_start):
            [rx_start_counters, tx_start_counters] = counters_start[node_iface]
            [rx_end_counters, tx_end_counters] = counters_end[node_iface]
            rx_delta = [end_i - start_i for start_i, end_i in zip(rx_start_counters, rx_end_counters)]
            tx_delta = [end_i - start_i for start_i, end_i in zip(tx_start_counters, tx_end_counters)]
            node_deltas[node_iface] = (rx_delta, tx_delta)

        return node_deltas
    
if __name__ == '__main__':
    
    sc = PortCounters()
    sc.run()