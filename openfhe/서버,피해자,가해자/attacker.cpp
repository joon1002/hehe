#include <iostream>
#include <sstream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main() {
    try {
        // Initialize Boost ASIO for network communication
        boost::asio::io_context io_context;

        // Server address and port
        std::string server_address = "34.123.52.9"; // Change to your server's IP address
        std::string server_port = "33333"; // Change to your server's listening port

        // Try to connect to the server
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server_address, server_port);
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        std::cout << "Connected to server" << std::endl;

        // Send identifier to server
        char client_type = 'A';
        boost::asio::write(socket, boost::asio::buffer(&client_type, sizeof(client_type)));

        // Input latitude and longitude
        double latitude, longitude;
        std::cout << "Enter latitude: ";
        std::cin >> latitude;
        std::cout << "Enter longitude: ";
        std::cin >> longitude;

        // Convert to integer with scaling
        int64_t intLatitude = static_cast<int64_t>(latitude * 100);
        int64_t intLongitude = static_cast<int64_t>(longitude * 100);

        // Serialize coordinates as plain text
        std::stringstream ss;
        ss << intLatitude << " " << intLongitude;
        std::string data = ss.str();
        uint32_t data_size = data.size();

        // Send the serialized coordinates
        boost::asio::write(socket, boost::asio::buffer(&data_size, sizeof(data_size)));
        boost::asio::write(socket, boost::asio::buffer(data));
        std::cout << "Sent coordinates to server" << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Exception in attacker client: " << e.what() << std::endl;
    }

    return 0;
}
