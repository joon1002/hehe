package com.example.client0511;

import android.os.AsyncTask;
import android.os.Bundle;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import android.view.View;

public class MainActivity extends AppCompatActivity {

    private TextView textView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textView = findViewById(R.id.textView);


    }
    // 버튼 클릭 시 호출되는 메서드
    public void connectToServer(View view) {
        new CommunicateWithServerTask().execute();
    }

    // 서버와 통신을 담당하는 AsyncTask
    private class CommunicateWithServerTask extends AsyncTask<Void, Void, String> {

        @Override
        protected String doInBackground(Void... voids) {
            String serverAddress = "35.202.222.91"; // 서버의 실제 IP 주소 또는 도메인 이름
            int serverPort = 12346; // 서버 포트 번호
            String name = "미니몬";
            String message = "안녕, 서버!";
            String request = name + "&&" + message;

            try {
                // 서버에 연결
                Socket socket = new Socket(serverAddress, serverPort);

                // 데이터 전송
                PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
                out.println(request);

                // 서버로부터 응답 받기
                BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                String response = in.readLine();

                // 소켓 닫기
                socket.close();

                return response;
            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        }

        @Override
        protected void onPostExecute(String response) {
            if (response != null) {
                // 응답을 텍스트 뷰에 설정
                textView.setText(response);
            } else {
                // 에러 메시지를 텍스트 뷰에 설정
                textView.setText("서버 통신 중 오류 발생");
            }
        }
    }
}
