import scipy.stats as sps
import numpy as np
from numpy.random import SeedSequence, default_rng, uniform, seed
import os
import shutil
import argparse
import pandas as pd
import matplotlib.pyplot as plt

class DistHandler:
    def __init__(self, name, params):
        self.dist_name = name
        self.params = params
        self.cdf, self.ppf, self.pdf, self.rvs = self.getDistribution(self.dist_name, self.params)
        self.rng = default_rng(SeedSequence().entropy)

    def getValues(self, n):
        return self.rvs(size=n, random_state=self.rng)

    def getDistribution(self, name, params):
        try:
            distribution = getattr(sps, name)
        except Exception:
            print(f"[ERROR]: Couldn't find {name}")
        arg = params[2:]
        loc = params[0]
        scale = params[1]
        v = distribution(loc=loc, scale=scale, *arg)
        cdf = v.cdf
        pdf = v.pdf
        ppf = v.ppf
        rvs = v.rvs
        return cdf, ppf, pdf, rvs

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some integers.')

    # FRAME SIZE
    parser.add_argument('-fsv', '--fs_values', dest='fs_values', nargs='+', type=float,
                        help='Values of the selected Frame Sizes model as arg1 arg2 ... loc scale', required=True)
    parser.add_argument('-fsm', '--fs_model', dest='fs_model', type=str,
                        help="Frame Sizes model's scipy.stats unique ID", required=True)

    # INTER FRAME TIMES
    parser.add_argument('-ifv', '--if_values', dest='if_values', nargs='+', type=float,
                        help='Values of the selected Inter Frames Times model as arg1 arg2 ... loc scale', required=True)
    parser.add_argument('-ifm', '--if_model', dest='if_model', type=str,
                        help="Inter Frames Times model's scipy.stats unique ID", required=True)

    # INTER PACKET TIMES
    parser.add_argument('-ipv', '--ip_values', dest='ip_values', nargs='+', type=float,
                        help='Values of the selected Frame Sizes model as arg1 arg2 ... loc scale', required=True)
    parser.add_argument('-ipm', '--ip_model', dest='ip_model', type=str,
                        help="Inter Packet Times model's scipy.stats unique ID", required=True)

    parser.add_argument('-sd', '--savedir', dest='savedir', type=str,
                        help="Output save directory", required=True)
    parser.add_argument('-of', '--outfilename', dest='outfilename', type=str,
                        help="Output filename", required=True)

    parser.add_argument('-t', '--time', dest='time', default=900, type=int,
                        help="Target time", required=True)
    parser.add_argument('-ps', '--packetsize', default=1442, dest='packetsize', type=int,
                        help="Packet size", required=True)

    parser.add_argument('-ow', '--overwrite', dest='overwrite', action='store_true',
                        help="If True, it overwrites the file if it exists. Other wise, its protected against overwrite")

    args = parser.parse_args()

    # Frame size
    frame_size = DistHandler(args.fs_model, args.fs_values)

    # Frame size
    frame_times = DistHandler(args.if_model, args.if_values)

    # Frame size
    packet_times = DistHandler(args.ip_model, args.ip_values)

    # File name
    target_dir = os.path.join(args.savedir, args.outfilename)
    traffic_file = os.path.join(target_dir, args.outfilename + ".txt")
    config_file = os.path.join(target_dir, args.outfilename + "_config.txt")
    overwrite = args.overwrite

    if os.path.exists(target_dir):
        if not overwrite:
            print(f"Protected against overwrite, and file [{target_dir}] already exists. To avoid this protection add -ow or --overwrite. Eexiting . . .")
            exit(1)
        else:
            print(f"Overwriting file [{target_dir}] that already exists. To avoid overwriting, remove -ow or --overwrite")
            shutil.rmtree(target_dir)
    
    try:
        os.mkdir(target_dir)
    except Exception:
        print(f"Wrong save directory [{target_dir}], exiting . . .")
        exit(1)

    target_time = args.time
    packet_size = args.packetsize

    # Not efficient but does the trick without the need of the original data
    mean_frame_size = np.mean(frame_size.getValues(1000000))
    print("Mean frame size: " + str(mean_frame_size))
    mean_frame_times = np.mean(frame_times.getValues(1000000))
    # Number of frames given the target generation time
    n_frames = int(target_time/mean_frame_times)

    # Total number of frames
    total_bytes = mean_frame_size*n_frames
    print("Total bytes: " + str(total_bytes))
    total_mb = total_bytes/125000
    print("Total MB: " + str(total_mb))
    mbps = total_mb/target_time
    print("MBPS:" + str(mbps))

    # Generate a generous quantity of packets. Is faster to do it this
    # way instead of per packet. The memory overhead is not big.
    n_packets = int(1.5*total_bytes/packet_size)

    # Generate as many frame sizes as estimated required frames
    sizes = frame_size.getValues(n_frames)

    # Generate as many inter frame times as estimated required frames
    inter_frames = frame_times.getValues(n_frames)

    # Estimate enough interpacket times
    inter_packets = packet_times.getValues(n_packets)

    # Open and create traffic file
    with open(traffic_file, "w") as file:
        t = 0.0
        p_counter = 0
        for frame in range(n_frames):
            f_size = int(sizes[frame])
            tt = t
            while f_size>0:
                if len(inter_packets)!=0 and p_counter < len(inter_packets):
                    p_time = inter_packets[p_counter]
                    p_size = max(0,min(packet_size, f_size))
                    file.write(f"{tt} {p_size}\n")
                    f_size -= packet_size
                    tt += p_time
                    p_counter += 1
                else:
                    file.write(f"{t} {f_size}\n")
                    f_size = 0
                    p_counter += 1
            f_time = inter_frames[frame]
            t += f_time

    # Open and create config file
    with open(config_file, "w") as file:
        file.write(f"Frame size data: {args.fs_model} {args.fs_values}\n")
        file.write(f"Inter frame times data : {args.if_model} {args.if_values}\n")
        file.write(f"Inter packet times data : {args.ip_model} {args.ip_values}\n")
        file.write(f"Target time: {target_time} s\n")
        file.write(f"Packet size: {packet_size} bytes\n")
        file.write(f"Mean frame size: {round(mean_frame_size, 2)} bytes\n")
        file.write(f"Total bytes: {round(total_bytes, 2)}\n")
        file.write(f"Total MB: {round(total_mb, 2)}\n")
        file.write(f"MBPS: {round(mbps, 2)}\n")

    # Draw traffic data
    traffic_data = pd.read_csv(traffic_file, sep=" ", header=None)
    plt.scatter(traffic_data[0], traffic_data[1])
    plt.xlabel("Time(s)")
    plt.ylabel("Traffic(bytes)")
    plt.savefig(f"{target_dir}/{args.outfilename}.png")
