#!/usr/bin/env python3
import sys
from http.server import HTTPServer, BaseHTTPRequestHandler

class BackendHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(f"Backend {self.server.server_port}\n".encode())
    
    def do_POST(self):
        self.do_GET()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 backend_server.py <port>")
        sys.exit(1)
    
    port = int(sys.argv[1])
    server = HTTPServer(('localhost', port), BackendHandler)
    print(f"Backend server running on port {port}")
    server.serve_forever()

