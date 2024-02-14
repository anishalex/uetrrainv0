# import socket

# def start_server(host='127.0.0.1', port=7766):
#     with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
#         s.bind((host, port))
#         s.listen()
#         print(f"Server started on {host}:{port}. Waiting for connection...")
#         conn, addr = s.accept()
#         with conn:
#             print(f"Connected by {addr}")
#             while True:
#                 data = conn.recv(1024)
#                 if not data:
#                     break
#                 print(f"Received data: {data}")
#                 # You can add more logic here to handle the data

# if __name__ == "__main__":
#     start_server()

import socket
import numpy as np
import cv2

# TCP/IP socket parameters
TCP_IP = '127.0.0.1'  # IP address
TCP_PORT = 7766      # Port number

# Image parameters (must match Unreal Engine settings)
WIDTH = 1024
HEIGHT = 1024
CHANNELS = 3
IMAGE_SIZE = WIDTH * HEIGHT * CHANNELS

# Create a socket object
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind and listen
server_socket.bind((TCP_IP, TCP_PORT))
server_socket.listen(1)

# Accept a connection
conn, addr = server_socket.accept()
print('Connection address:', addr)

while True:
    # Receive data
    data = conn.recv(IMAGE_SIZE)

    # Check if we received the right amount of data
    if len(data) == IMAGE_SIZE:
        # Convert data to a numpy array and reshape it to an image
        image = np.frombuffer(data, dtype=np.uint8).reshape(HEIGHT, WIDTH, CHANNELS)

        image  = image[:, :, ::-1]
        # Display the image
        cv2.imshow('frame', image)

        # Break the loop with 'q'
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    else:
        print(f"Incomplete data received ({len(data)}b). Skipping frame.")

# Clean up
conn.close()
server_socket.close()
cv2.destroyAllWindows()
