from simple-websocket-server import WebSocketServer, WebSocket

class ControlRobot(WebSocket):
	def handle(self):
		# move robot

	def connected(self):
		# initialize robot
		
	def handle_close(self):
		print(self.address, 'closed')

server = WebSocketServer('', 8000, ControlRobot)
server.serve_forever()
