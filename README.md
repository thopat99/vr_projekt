# vr_projekt
These are all source files that were created in the "VR-Projekt" in the Summer Term 2022.

## Arduino
Source File of the Arduino UNO which takes the parameters from the Pi and converts them to a Movement-Command for the Braccio.

## Raspi
Simple Python HTTP-Server that takes the Parameters of the HTTP-Requests and sends them via Serial to the Arduino.
Start the Server via:

`python3 http_server.py`

Additionally there is a WebSocket-Client that could also be used for the communication (including an HTML-Page that establishes a connection to the WebSocket)

## Win
Visual Studio Project than connects to the Phantom Premium, reads its angles values, calculates the angles for the Braccio and sends an HTTP-Request to the RasPi. To start the application, the Visual Studio Project has to be openend and then the application can be run from it.
