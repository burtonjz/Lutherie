import argparse
import json
from socket import *
import time, signal, struct, threading

TERMINATE = False

HEADER_FORMAT = "<IIQ"  # uint32 componentId, uint32 channel, uint64 size
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 16 bytes

CONTROL_HOST = "localhost"
CONTROL_PORT = 12345
DATA_HOST    = "localhost"
DATA_PORT    = 12346


def load_commands(path):
    """
    Load a JSON file containing a list of command objects and turn each
    one into a newline-terminated JSON byte string, matching the format
    the rest of the script expects.
    """
    with open(path, "r") as f:
        data = json.load(f)

    if not isinstance(data, list):
        raise ValueError("Commands JSON file must contain a list of command objects")

    commands = []
    for entry in data:
        line = json.dumps(entry) + "\n"
        commands.append(line.encode())
    return commands


def signal_handling(signum, frame):
    global TERMINATE
    TERMINATE = True


def recv_exact(sock, n):
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError(f"Socket closed after {len(buf)}/{n} bytes")
        buf += chunk
    return bytes(buf)


def read_data(sock):
    raw_header = recv_exact(sock, HEADER_SIZE)
    component_id, channel, size = struct.unpack(HEADER_FORMAT, raw_header)
    print(f"  componentId  = {component_id}")
    print(f"  channel      = {channel}")
    print(f"  payload size = {size} bytes ({size // 8} values)")
    values = []
    if size > 0:
        raw_data = recv_exact(sock, size)
        n_values = size // 8
        values = list(struct.unpack(f"<{n_values}d", raw_data))
        print(f"  first 5 values: {values[:5]}")
    return component_id, channel, values


def data_listener(data_sock):
    """Runs in a background thread, printing every incoming data message."""
    print("[data] Listener started.")
    msg_count = 0
    try:
        while not TERMINATE:
            print(f"\n[data] --- Message {msg_count + 1} ---")
            read_data(data_sock)
            msg_count += 1
    except ConnectionError as e:
        print(f"[data] Connection closed: {e}")
    except Exception as e:
        print(f"[data] Unexpected error: {e}")
    finally:
        print("[data] Listener stopped.")


def main():
    parser = argparse.ArgumentParser(
        description="Send a sequence of JSON control commands to the control server."
    )
    parser.add_argument(
        "commands_file",
        help="Path to a .json file containing a list of command objects",
    )
    args = parser.parse_args()

    commands = load_commands(args.commands_file)

    # --- connect both sockets ---
    print(f"[ctrl] Connecting to {CONTROL_HOST}:{CONTROL_PORT} ...")
    control = socket()
    control.connect((CONTROL_HOST, CONTROL_PORT))
    print("[ctrl] Connected.")

    print(f"[data] Connecting to {DATA_HOST}:{DATA_PORT} ...")
    data = socket()
    data.connect((DATA_HOST, DATA_PORT))
    print("[data] Connected.\n")

    # --- start data listener in background ---
    listener = threading.Thread(target=data_listener, args=(data,), daemon=True)
    listener.start()

    # --- send control commands ---
    for cmd in commands:
        print(f"[ctrl] Sending: {cmd.decode().strip()}")
        control.sendall(cmd)
        time.sleep(.5)

    # --- wait until Ctrl-C ---
    signal.signal(signal.SIGINT, signal_handling)
    print("\n[ctrl] All commands sent. Listening for data (Ctrl-C to stop)...\n")
    while not TERMINATE:
        time.sleep(0.5)

    # --- clean shutdown ---
    print("\n[ctrl] Stopping...")
    control.sendall(b'{"action":"set_state","state":"stop"}\n')
    control.close()
    data.close()
    print("Done.")


if __name__ == "__main__":
    main()