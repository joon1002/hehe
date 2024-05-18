import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {
    private static final int SERVER_PORT = 10716;
    private static final double EARTH_RADIUS = 6371.01; // 지구 반지름 (킬로미터)

    public static void main(String[] args) {
        try (ServerSocket serverSocket = new ServerSocket(SERVER_PORT)) {
            System.out.println("[] Listening on port " + SERVER_PORT);

            // 두 개의 클라이언트 연결 대기
            Socket[] clientSockets = new Socket[2];
            for (int i = 0; i < 2; i++) {
                Socket clientSocket = serverSocket.accept();
                System.out.println("[] Accepted connection from " + clientSocket.getInetAddress() + ":" + clientSocket.getPort());
                clientSockets[i] = clientSocket;
            }

            // 각 클라이언트로부터 위도, 경도 및 문자열을 받음
            double[] latitudes = new double[2];
            double[] longitudes = new double[2];
            String[] strings = new String[2];
            for (int i = 0; i < 2; i++) {
                DataInputStream inputStream = new DataInputStream(clientSockets[i].getInputStream());
                latitudes[i] = inputStream.readDouble();
                longitudes[i] = inputStream.readDouble();
                strings[i] = inputStream.readUTF();
                System.out.println("Client " + (i + 1) + " Latitude: " + latitudes[i] + ", Longitude: " + longitudes[i] + ", String: " + strings[i]);
            }

            // 위도와 경도의 차이를 계산하여 거리 구함
            double distance = calculateHaversineDistance(latitudes[0], longitudes[0], latitudes[1], longitudes[1]);
            System.out.println("Calculated Distance using Haversine formula: " + distance + " km");

            // 문자열이 '가해자'인 클라이언트에게 거리 전송 및 수정된 거리 수신
            double modifiedDistance = 0;
            for (int i = 0; i < 2; i++) {
                if ("가해자".equals(strings[i])) {
                    DataOutputStream outputStream = new DataOutputStream(clientSockets[i].getOutputStream());
                    outputStream.writeDouble(distance);
                    outputStream.flush();

                    // 클라이언트로부터 수정된 거리 수신
                    modifiedDistance = new DataInputStream(clientSockets[i].getInputStream()).readDouble();
                    System.out.println("Received modified distance from 가해자: " + modifiedDistance);
                }
            }

            // 수정된 거리를 피해자에게 전송
            for (int i = 0; i < 2; i++) {
                if ("피해자".equals(strings[i])) {
                    DataOutputStream outputStream = new DataOutputStream(clientSockets[i].getOutputStream());
                    outputStream.writeDouble(modifiedDistance);
                    outputStream.flush();
                    System.out.println("Sent modified distance to 피해자: " + modifiedDistance);
                }
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // 하버사인 공식을 이용하여 두 지점 간의 거리를 계산하는 메서드
    private static double calculateHaversineDistance(double lat1, double lon1, double lat2, double lon2) {
        double lat1Rad = Math.toRadians(lat1);
        double lon1Rad = Math.toRadians(lon1);
        double lat2Rad = Math.toRadians(lat2);
        double lon2Rad = Math.toRadians(lon2);

        double deltaLat = lat2Rad - lat1Rad;
        double deltaLon = lon2Rad - lon1Rad;

        double a = Math.sin(deltaLat / 2) * Math.sin(deltaLat / 2)
                + Math.cos(lat1Rad) * Math.cos(lat2Rad)
                * Math.sin(deltaLon / 2) * Math.sin(deltaLon / 2);
        double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

        return EARTH_RADIUS * c;
    }
}
