User
import socket

SERVERHOST = '0.0.0.0'  # 모든 네트워크 인터페이스에서 연결 허용
SERVERPORT = yourserverport

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((SERVER_HOST, SERVER_PORT))
        server_socket.listen(1)
        print(f"[] Listening on {SERVER_HOST}:{SERVER_PORT}")

        while True:
            client_socket, client_address = server_socket.accept()
            print(f"[] Accepted connection from {client_address[0]}:{client_address[1]}")

            with client_socket:
                try:
                    # 클라이언트로부터 위도와 경도를 받음
                    latitude = client_socket.recv(1024).decode('utf-8')
                    longitude = client_socket.recv(1024).decode('utf-8')
                    print(f"Latitude: {latitude}, Longitude: {longitude}")

                except ConnectionResetError:
                    print("Connection closed by client")

if __name == "__main":
    main()
