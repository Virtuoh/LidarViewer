# UE UDP Receiver Plugin

Simple Unreal Engine (UE) plugin containing an actor component which creates a UDP receiver socket for inter-communication.

![alt text](Resources/blueprint-example-1.png)

![alt text](Resources/blueprint-example-2.png)

## Features
- Event-driven UDP message processing
- Send message functionality for client communication
- Blueprint integration
- Customizable IP, port, and buffer size

## Dependencies
Plugin makes use of `Networking` and `Sockets` UE modules.

## Installation
1. Clone this repository into your `Plugins/` directory:
   ```bash
   $ git clone https://github.com/wjake/ue-udp-receiver.git Plugins/UDPReceiver
   ```
2. Open your Unreal Engine project, and enable the plugin in the Plugins Manager.

## Usage
### Unreal Engine
1. Add the `UUDPReceiver` component to your actor
2. Configure the properties (IP, Port, etc.)
3. Set component `Auto activate` in Details pane, or call `StartUDPReceiver` in the Event Graph
4. Use `OnMessageReceivedEvent` node to handle received messages
5. Use `SendMessage` node to send message to a client

### Client Example (Python)
1. Import `socket` library dependency
```Python
import socket
```
2. Define socket IP address & port number
```Python
server_address = ('127.0.0.1', 65432)
```
3. Create connection to UDP socket
```Python
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
```
4. Send data
```Python
udp_socket.sendto('data', server_address)
```
5. Receive Message
```Python
data, server = udp_socket.recvfrom(1024)
```
6. Close socket connection
```Python
udp_socket.close()
```
