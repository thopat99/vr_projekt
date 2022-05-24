from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse
import json, serial

hostName = '0.0.0.0'
serverPort = 4200

class VRHTTPServer(BaseHTTPRequestHandler):

    def do_GET(self):

        if (parse_qs(urlparse(self.path).query).get('command', None) != None):
            message = parse_qs(urlparse(self.path).query).get('command', None)[0]
            print(message)

            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()

            json_string = {'command' : message}

            self.wfile.write(json.dumps(json_string).encode(encoding='utf-8'))
            
            ser.write(bytes(message, 'utf-8'))

        return

if __name__ == "__main__":
    webServer = HTTPServer((hostName, serverPort), VRHTTPServer)
    ser = serial.Serial('/dev/ttyACM0',38400, timeout=1)
    ser.flush()

    print("Server started http://%s:%s" % (hostName, serverPort))
    ser.write(b"neutral")

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")