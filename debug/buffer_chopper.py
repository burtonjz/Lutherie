from socket import *
import time, signal

terminate = False

def signal_handling(signum,frame):           
    global terminate                         
    terminate = True 

s = socket()
s.connect(('localhost',12345))

commands = [
    b'{"action":"set_midi_device","device_id":2}\n',
    b'{"action":"set_audio_device","device_id":129}\n',
    b'{"action":"add_component","name":"File Buffer"}\n', # id 0
    b'{"action":"set_file_path","componentId":0,"path":"/home/burtonjz/Downloads/chopper-test.mp3"}\n',
    b'{"action":"add_component","name":"Buffer Chopper"}\n', # id 1
    b'{"action":"add_component","name":"Buffer Streamer"}\n', # id 2
    b'{"action":"create_connection","inbound":{"componentId":1,"index":0,"socketType":"Buffer Inbound"},"outbound":{"componentId":0,"index":0,"socketType":"Buffer Outbound"}}\n',
    b'{"action":"create_connection","inbound":{"componentId":2,"index":0,"socketType":"Buffer Inbound"},"outbound":{"componentId":1,"index":0,"socketType":"Buffer Outbound"}}\n',
    b'{"action":"create_connection","inbound":{"index":0,"socketType":"Signal Inbound"},"outbound":{"componentId":2,"index":0,"socketType":"Signal Outbound"}}\n',
    b'{"action":"set_parameter","componentId":1,"parameter":"Start","value": 480000}\n',
    b'{"action":"set_parameter","componentId":1,"parameter":"Duration","value": 960000}\n',
    b'{"action":"set_state","state":"run"}\n'
]       

for c in commands:
    s.sendall(c)
    time.sleep(2)

signal.signal(signal.SIGINT, signal_handling)
while (True):
    time.sleep(2)
    if terminate:
        break

s.sendall(b'{"action":"set_state","state":"stop"}\n')
s.close()