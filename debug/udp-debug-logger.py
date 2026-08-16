import argparse
import csv
import os
import struct
import time
import signal
from collections import defaultdict
from socket import *

import numpy as np

TERMINATE = False

UDP_HEADER_FORMAT = "<i"  # int32 componentId
UDP_HEADER_SIZE = struct.calcsize(UDP_HEADER_FORMAT)
UDP_RECV_BUFSIZE = 65536  # should be bigger than any udp payload
UDP_PORT = 54322

def signal_handling(signum, frame):
    global TERMINATE
    TERMINATE = True


def parse_udp_packet(raw):
    """
    Parse a UDP payload of the form:
        int32 componentId
        float32[]  (remaining bytes)
    Returns (component_id, [float, ...]).
    """
    if len(raw) < UDP_HEADER_SIZE:
        raise ValueError(f"Packet too short for header: {len(raw)} bytes")

    (component_id,) = struct.unpack_from(UDP_HEADER_FORMAT, raw, 0)

    payload = raw[UDP_HEADER_SIZE:]
    if len(payload) % 4 != 0:
        raise ValueError(f"Payload size {len(payload)} is not a multiple of 4, which suggests it's not valid float32...")

    n_values = len(payload) // 4
    values = list(struct.unpack(f"<{n_values}f", payload)) if n_values else []
    return component_id, values


def main():
    parser = argparse.ArgumentParser(
        description="Standalone UDP debug listener. Logs full packet data per component "
                    "for waveform/FFT analysis (CSV live, .npy on exit)."
    )
    parser.add_argument("--host", default="127.0.0.1", help="host address")
    parser.add_argument("--port", type=int, default=UDP_PORT, help="UDP port to listen on (default: 54322)")
    parser.add_argument("--out-dir", default="udp_capture", help="Directory to write per-component data into")
    parser.add_argument("--max-values-printed", type=int, default=5, help="num preview values to display in terminal (default: 5)")
    args = parser.parse_args()

    signal.signal(signal.SIGINT, signal_handling)
    os.makedirs(args.out_dir, exist_ok=True)

    sock = socket(AF_INET, SOCK_DGRAM)
    sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))
    sock.settimeout(0.5)  # don't block forever so we can check termination

    print(f"[udp] Listening on {args.host}:{args.port}")
    print(f"[udp] Writing per-component CSV + .npy into: {args.out_dir}/")
    print("[udp] Press Ctrl-C to stop.\n")

    # one file for each component id, new files created if new components start streaming through
    # Row format: timestamp, n_values, values...
    csv_files = {}
    csv_writers = {}

    # In-memory accumulation for the final .npy dump per componentId.
    accumulated = defaultdict(list)   

    def get_csv_writer(component_id):
        if component_id not in csv_writers:
            path = os.path.join(args.out_dir, f"component_{component_id}.csv")
            f = open(path, "a", newline="")
            w = csv.writer(f)
            csv_files[component_id] = f
            csv_writers[component_id] = w
        return csv_writers[component_id]

    pkt_count = 0
    error_count = 0
    error_log_path = os.path.join(args.out_dir, "errors.log")
    error_log = open(error_log_path, "a")

    try:
        while not TERMINATE:
            try:
                raw, addr = sock.recvfrom(UDP_RECV_BUFSIZE)
            except timeout:
                continue
            except OSError as e:
                print(f"[udp] Socket error: {e}")
                break

            recv_time = time.time()
            pkt_count += 1

            try:
                component_id, values = parse_udp_packet(raw)
            except ValueError as e:
                error_count += 1
                error_log.write(f"{recv_time:.6f}\tfrom={addr}\t{e}\traw_len={len(raw)}\n")
                error_log.flush()
                print(f"[udp] #{pkt_count} MALFORMED from {addr}:\t{e}")
                continue

            # write to csv file
            writer = get_csv_writer(component_id)
            writer.writerow([f"{recv_time:.6f}", len(values), *values])
            csv_files[component_id].flush()

            # accumulate for final .npy dump 
            accumulated[component_id].append((recv_time, values))

            # update terminal, once every 50 packets
            if pkt_count % 50 == 0 or pkt_count == 1:
                preview = values[: args.max_values_printed]
                print(
                    f"[udp] #{pkt_count} componentId={component_id} "
                    f"n_values={len(values)} preview={preview}"
                )

    finally:
        sock.close()
        for f in csv_files.values():
            f.close()
        error_log.close()

        # write out numpy files for each component id
        # column 0 = timestamp, columns 1: = float values (NaN-padded to max width per component)
        for component_id, rows in accumulated.items():
            max_len = max(len(v) for _, v in rows)
            arr = np.full((len(rows), 1 + max_len), np.nan, dtype=np.float64)
            for i, (ts, values) in enumerate(rows):
                arr[i, 0] = ts
                arr[i, 1 : 1 + len(values)] = values
            npy_path = os.path.join(args.out_dir, f"component_{component_id}.npy")
            np.save(npy_path, arr)
            print(f"[udp] Saved {npy_path} shape={arr.shape} (col0=timestamp, col1:=values)")

        print(f"\n[udp] Stopped. Total packets: {pkt_count}, malformed: {error_count}")
        print(f"[udp] Per-component data in: {args.out_dir}/  (CSV live, .npy on exit)")


if __name__ == "__main__":
    main()