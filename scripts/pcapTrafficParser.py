import os
import argparse

from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, TCP, UDP
from scapy.all import *
from scapy.layers.rtp import RTP

def process_pcap(file_name, outfile, tpe):
    print('Opening {}...'.format(file_name))

    init_t = 0
    pkts = PcapReader(file_name)
    prev = 0
    count = 0
    ts = {}
    for pkt_data in pkts:
        count += 1
        try:
            ether_pkt = pkt_data[Ether]
            if 'type' not in ether_pkt.fields:
                # LLC frames will have 'len' instead of 'type'.
                # We disregard those
                continue

            if ether_pkt.type != 0x0800:
                # disregard non-IPv4 packets
                continue
        except Exception:
            pass
        try:
            ip_pkt = pkt_data[IP]
        except Exception:
            continue
        if (ip_pkt.proto != 6 and tpe==TCP) or (ip_pkt.proto != 17 and tpe==UDP) :
            # Ignore non-TCP packet
            continue
        pkt = pkt_data[tpe]
        if tpe==TCP and len(pkt.options) == 3:
            print("Options: " + str(pkt.options[-1][-1][-1]))
        if tpe == TCP:
            if ip_pkt.src != ip:
                continue

        if (ip_pkt.flags == 'MF') or (ip_pkt.frag != 0):
            print('No support for fragmented IP packets')
            break
        port = pkt.dport
        if port in ports:
            try:
                ip_pkt.payload = RTP(pkt_data["Raw"].load)

                # packet[RTP].version = 0
                # packet[RTP].padding = 0
                # packet[RTP].extension = 0
                # packet[RTP].numsync = 0
                # packet[RTP].marker = 0
                # packet[RTP].payload_type = 0
                # packet[RTP].sequence = 0

                # packet[RTP].timestamp = 0
                if port not in ts:
                    pass
                else:
                    pass
                ts[port] = pkt_data[RTP].timestamp
            except IndexError:
                pass
            pl = ip_pkt.len
            if init_t==0:
                init_t = pkt_data.time
            t = pkt_data.time - init_t
            outfile.write(f"{t} {pl}\n")
            v = int(100*t/900)
            if v > prev:
                print("Processed " + str(v)+ "%")
                prev = v

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some integers.')

    # FRAME SIZE
    parser.add_argument('-p', '--ports', dest='ports', nargs='+', type=int,
                        help='Target ports given as arg1 arg2 ... loc scale', required=True)
    parser.add_argument('-ptcl','--protocol', dest='protocol', type=str, help="Target IP protocol [TCP,UDP]", required=True, choices=["UDP", "TCP", "udp", "tcp"])

    parser.add_argument('-pc', '--pcap', dest='pcap', type=str, help="Input .PCAP file" , required=True)
    parser.add_argument('-sd', '--savedir', dest='savedir', type=str,
                        help="Output save directory", required=True)
    parser.add_argument('-of', '--outfilename', dest='outfilename', type=str,
                        help="Output filename", required=True)
    parser.add_argument('-ow', '--overwrite', dest='overwrite', action='store_true',
                        help="If True, it overwrites the file if it exists. Other wise, its protected against overwrite")
    args = parser.parse_args()

    # File name
    target_dir = args.savedir
    filename = args.outfilename
    overwrite = args.overwrite
    if os.path.exists(target_dir):
        save_dir = os.path.join(target_dir, filename)
    else:
        try:
            os.mkdir(target_dir)
        except Exception:
            print(f"Wrong save directory [{target_dir}], exiting . . .")
            exit(1)
    if os.path.exists(save_dir):
        if not overwrite:
            print(f"Protected against overwrite, and file [{save_dir}] already exists. To avoid this protection add -ow or --overwrite. Eexiting . . .")
            exit(1)
        else:
            print(f"Overwriting file [{save_dir}] that already exists. To avoid overwriting, remove -ow or --overwrite")
            os.remove(save_dir)

    ports = args.ports
    pcap = args.pcap
    protocol = UDP if args.protocol.lower() == "udp" else TCP

    # Open and create file
    with open(save_dir, "w") as outfile:
        process_pcap(pcap, outfile, protocol)


