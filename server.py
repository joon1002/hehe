import socket
import math

SERVER_PORT = 10716
EARTH_RADIUS = 6371.01  # 지구 반지름 (킬로미터)

def calculate_haversine_distance(lat1, lon1, lat2, lon2):
    lat1_rad = math.radians(lat1)
    lon1_rad = math.radians(lon1)
    lat2_rad = math.radians(lat2)
    lon2_rad = math.radians(lon2)

    delta_lat = lat2_rad - lat1_rad
    delta_lon = lon2_rad - lon1_rad

    a = math.sin(delta_lat / 2) ** 2 + math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))

    return EARTH_RADIUS * c

def handle_client_connections(client_sockets):
    latitudes = []
    longitudes = []
    strings = []
    for client_socket in client_sockets:
        with client_socket.makefile('r') as input_stream:
            latitudes.append(float(input_stream.readline().strip()))
            longitudes.append(float(input_stream.readline().strip()))
            strings.append(input_stream.readline().strip())
        print(f"Client Latitude: {latitudes[-1]}, Longitude: {longitudes[-1]}, String: {strings[-1]}")

    distance = calculate_haversine_distance(latitudes[0], longitudes[0], latitudes[1], longitudes[1])
    print(f"Calculated Distance using Haversine formula: {distance} km")

    modified_distance = 0
    for i in range(2):
        if strings[i] == "가해자":
            with client_sockets[i].makefile('w') as output_stream:
                output_stream.write(f"{distance}\n")
                output_stream.flush()
            with client_sockets[i].makefile('r') as input_stream:
                modified_distance = float(input_stream.readline().strip())
            print(f"Received modified distance from 가해자: {modified_distance}")

    for i in range(2):
        if strings[i] == "피해자":
            with client_sockets[i].makefile('w') as output_stream:
                output_stream.write(f"{modified_distance}\n")
                output_stream.flush()
            print(f"Sent modified distance to 피해자: {modified_distance}")

    for client_socket in client_sockets:
        client_socket.close()

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('', SERVER_PORT))
    server_socket.listen(2)  # 두 개의 클라이언트 연결 대기
    print(f"[] Listening on port {SERVER_PORT}")

    try:
        while True:
            client_sockets = []
            for i in range(2):
                client_socket, addr = server_socket.accept()
                print(f"[] Accepted connection from {addr}")
                client_sockets.append(client_socket)
            
            handle_client_connections(client_sockets)

    except KeyboardInterrupt:
        print("Server shutting down.")
    finally:
        server_socket.close()

if __name__ == "__main__":
    main()
