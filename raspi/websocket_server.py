from simple-websocket-server import WebSocketServer, WebSocket
import serial

class ControlRobot(WebSocket):
	def handle(self):
		print(self.data)
		#ser.write(bytes(command, 'utf-8'))

	def connected(self):
		ser.write(b"neutral")
		
	def handle_close(self):
		print(self.address, 'closed')

ser = serial.Serial('/dev/ttyACM0',38400, timeout=1)
ser.flush()
server = WebSocketServer('', 8000, ControlRobot)
server.serve_forever()
