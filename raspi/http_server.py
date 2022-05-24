from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

hostName = '0.0.0.0'
serverPort = 4200

class VRHTTPServer(BaseHTTPRequestHandler):

    def do_GET(self):

        if (parse_qs(urlparse(self.path).query).get('command', None) != None):
            message = parse_qs(urlparse(self.path).query).get('command', None)[0]
            print(message)
            

        return

if __name__ == "__main__":
    webServer = HTTPServer((hostName, serverPort), VRHTTPServer)

    print("Server started http://%s:%s" % (hostName, serverPort))

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")