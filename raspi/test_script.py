import serial

if __name__ == '__main__':
	ser = serial.Serial('/dev/ttyACM0',38400, timeout=1)
	ser.flush()

	while True:
		command = input("Please enter the command: ")
		command += "\n"
		ser.write(bytes(command, 'utf-8'))
