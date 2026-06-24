from socket import *
import time, signal, struct, threading

TERMINATE = False
HEADER_FORMAT = "<IIQ"  # uint32 componentId, uint32 channel, uint64 size
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 16 bytes

CONTROL_HOST = "localhost"
CONTROL_PORT = 12345

DATA_HOST    = "localhost"
DATA_PORT    = 12346

commands = [
    b'{"action":"set_midi_device","device_id":2}\n',
    b'{"action":"set_audio_device","device_id":129}\n',
    b'{"action":"add_component","name":"File Buffer"}\n',         # id 0
    b'{"action":"add_component","name":"Buffer Chopper"}\n',      # id 1
    b'{"action":"add_component","name":"Buffer Streamer"}\n',     # id 2
    b'{"action":"set_file_path","componentId":0,"path":"/home/burtonjz/Downloads/chopper-test.mp3"}\n',
    b'{"action":"create_connection","inbound":{"componentId":1,"index":0,"socketType":"Buffer Inbound"},"outbound":{"componentId":0,"index":0,"socketType":"Buffer Outbound"}}\n',
    b'{"action":"create_connection","inbound":{"componentId":2,"index":0,"socketType":"Buffer Inbound"},"outbound":{"componentId":1,"index":0,"socketType":"Buffer Outbound"}}\n',
    b'{"action":"create_connection","inbound":{"index":0,"socketType":"Signal Inbound"},"outbound":{"componentId":2,"index":0,"socketType":"Signal Outbound"}}\n',
    b'{"action":"set_parameter","componentId":1,"parameter":"Start","value": 480000}\n',
    b'{"action":"set_parameter","componentId":1,"parameter":"Duration","value": 960000}\n',
    # b'{"action":"get_buffer_data","componentId":1,"channel":0}\n',
    b'{"action":"set_state","state":"run"}\n',
]


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
    time.sleep(2)

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