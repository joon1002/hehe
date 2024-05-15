import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;


public class Server {
    private static final int SERVER_PORT = 10716;
    private static final double EARTH_RADIUS = 6371e3; // 지구 반지름 (미터)

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

            // 위도와 경도의 차이를 계산하여 두 지점 간의 거리를 메터 단위로 계산
            double latitudeDifference = Math.toRadians(latitudes[1] - latitudes[0]);
            double longitudeDifference = Math.toRadians(longitudes[1] - longitudes[0]);
            double a = Math.sin(latitudeDifference / 2) * Math.sin(latitudeDifference / 2)
                    + Math.cos(Math.toRadians(latitudes[0])) * Math.cos(Math.toRadians(latitudes[1]))
                    * Math.sin(longitudeDifference / 2) * Math.sin(longitudeDifference / 2);
            double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
            double distance = EARTH_RADIUS * c;

            System.out.println("Calculated Distance: " + distance + " meters");

            // 문자열이 '가해자'인 클라이언트에게 거리 전송
            for (int i = 0; i < 2; i++) {
                if ("가해자".equals(strings[i])) {
                    DataOutputStream outputStream = new DataOutputStream(clientSockets[i].getOutputStream());
                    outputStream.writeDouble(distance);
                    outputStream.flush();

                    // 클라이언트로부터 수정된 거리 수신
                    double modifiedDistance = new DataInputStream(clientSockets[i].getInputStream()).readDouble();
                    System.out.println("Received modified distance from 가해자: " + modifiedDistance);
                }
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
