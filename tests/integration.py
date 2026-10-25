"""Run with: python3 tests/integration.py /path/to/iot-http-adapter"""
import base64
import json
import os
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

received = []

class Upstream(BaseHTTPRequestHandler):
    def do_POST(self):
        body = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        received.append((self.path, body))
        self.send_response(201)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(b'{"status":"ok"}')

    def log_message(self, *args):
        pass


def request(path, body=None):
    data = None if body is None else json.dumps(body).encode()
    req = urllib.request.Request('http://127.0.0.1:8080' + path, data=data,
                                 headers={'Content-Type': 'application/json'})
    try:
        with urllib.request.urlopen(req, timeout=8) as response:
            return response.status, response.read()
    except urllib.error.HTTPError as error:
        return error.code, error.read()


server = ThreadingHTTPServer(('127.0.0.1', 0), Upstream)
threading.Thread(target=server.serve_forever, daemon=True).start()
url = 'http://127.0.0.1:' + str(server.server_port)
try:
    for algorithm in ('base64', 'aes-256-gcm'):
        env = dict(os.environ, IOT_SENSOR_ALGORITHM=algorithm,
                   IOT_AES_KEY_BASE64=base64.b64encode(bytes(32)).decode(),
                   IOT_ADAPTER_HOST='127.0.0.1', IOT_CORE_URL=url, IOT_SENSOR_URL=url)
        process = subprocess.Popen([sys.argv[1]], env=env, stdout=subprocess.DEVNULL)
        try:
            for _ in range(100):
                if process.poll() is not None:
                    raise RuntimeError('Adapter exited during startup')
                try:
                    if request('/ping') == (200, b'pong'):
                        break
                except OSError:
                    time.sleep(.05)
            else:
                raise RuntimeError('Adapter did not start')
            data = {'sensor_id': 'sensor-1', 'value': 23.5, 'unit': 'C'}
            assert request('/api/v1/commands', data)[0] == 201
            path, envelope = received[-1]
            assert path == '/api/v1/commands'
            assert envelope['algorithm'] == algorithm
            assert request('/api/v1/ingest', envelope)[0] == 201
            assert received[-1] == ('/api/v1/ingest', data)
            count = len(received)
            assert request('/api/v1/ingest', data)[0] == 400
            assert request('/api/v1/ingest', dict(envelope, payload='!!!!'))[0] == 400
            assert request('/api/v1/ingest', dict(envelope, algorithm='unknown'))[0] == 400
            if algorithm == 'aes-256-gcm':
                packed = bytearray(base64.b64decode(envelope['payload']))
                packed[-1] ^= 1
                assert request('/api/v1/ingest', dict(envelope, payload=base64.b64encode(packed).decode()))[0] == 400
            assert len(received) == count
        finally:
            process.terminate()
            process.wait(timeout=5)
    print('HTTP integration tests passed (Base64 and AES-256-GCM)')
finally:
    server.shutdown()
    server.server_close()
