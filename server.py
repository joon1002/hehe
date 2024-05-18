import socket
import struct
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

    a = math.sin(delta_lat / 2) * math.sin(delta_lat / 2) + math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) * math.sin(delta_lon / 2)
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))

    return EARTH_RADIUS * c

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind(('', SERVER_PORT))
        server_socket.listen(2)
        print(f"[] Listening on port {SERVER_PORT}")

        client_sockets = []
        for i in range(2):
            client_socket, addr = server_socket.accept()
            print(f"[] Accepted connection from {addr[0]}:{addr[1]}")
            client_sockets.append(client_socket)

        latitudes = []
        longitudes = []
        strings = []

        for i in range(2):
            with client_sockets[i]:
                data = client_sockets[i].recv(1024)
                lat, lon, string = struct.unpack('ff20s', data)
                string = string.decode('utf-8').strip('\x00')
                latitudes.append(lat)
                longitudes.append(lon)
                strings.append(string)
                print(f"Client {i + 1} Latitude: {lat}, Longitude: {lon}, String: {string}")

        distance = calculate_haversine_distance(latitudes[0], longitudes[0], latitudes[1], longitudes[1])
        print(f"Calculated Distance using Haversine formula: {distance} km")

        modified_distance = 0
        for i in range(2):
            if strings[i] == "가해자":
                with client_sockets[i]:
                    client_sockets[i].sendall(struct.pack('f', distance))
                    modified_distance = struct.unpack('f', client_sockets[i].recv(1024))[0]
                    print(f"Received modified distance from 가해자: {modified_distance}")

        for i in range(2):
            if strings[i] == "피해자":
                with client_sockets[i]:
                    client_sockets[i].sendall(struct.pack('f', modified_distance))
                    print(f"Sent modified distance to 피해자: {modified_distance}")

if __name__ == "__main__":
    main()
