#include "openfhe.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <boost/asio.hpp>
#include <iomanip> // for std::fixed and std::setprecision

using namespace lbcrypto;
using boost::asio::ip::tcp;

double haversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371000; // Earth's radius in meters
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;

    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1) * cos(lat2) *
               sin(dLon / 2) * sin(dLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c;
}

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
        char client_type = 'V';
        boost::asio::write(socket, boost::asio::buffer(&client_type, sizeof(client_type)));

        // CryptoContext setup
        CCParams<CryptoContextBFVRNS> parameters;
        parameters.SetPlaintextModulus(65537);
        parameters.SetMultiplicativeDepth(1);
        parameters.SetRingDim(8192); // Adjust to satisfy the condition
        parameters.SetSecurityLevel(HEStd_128_classic);

        CryptoContext<DCRTPoly> cryptoContext = GenCryptoContext(parameters);
        cryptoContext->Enable(PKE);
        cryptoContext->Enable(KEYSWITCH);
        cryptoContext->Enable(LEVELEDSHE);

        // Key Generation
        auto keys = cryptoContext->KeyGen();
        cryptoContext->EvalMultKeyGen(keys.secretKey);

        // Input latitude and longitude
        double latitude, longitude;
        std::cout << "Enter latitude: ";
        std::cin >> latitude;
        std::cout << "Enter longitude: ";
        std::cin >> longitude;

        // Convert to integer with scaling
        int64_t intLatitude = static_cast<int64_t>(latitude * 100);
        int64_t intLongitude = static_cast<int64_t>(longitude * 100);

        // Encrypt data
        std::vector<int64_t> coordinates = {intLatitude, intLongitude};
        Plaintext plaintext = cryptoContext->MakePackedPlaintext(coordinates);
        auto ciphertext = cryptoContext->Encrypt(keys.publicKey, plaintext);

        // Serialize ciphertext
        std::stringstream ss;
        Serial::Serialize(ciphertext, ss, SerType::BINARY);
        std::string data = ss.str();
        uint32_t data_size = data.size();

        // Send the serialized ciphertext
        boost::asio::write(socket, boost::asio::buffer(&data_size, sizeof(data_size)));
        boost::asio::write(socket, boost::asio::buffer(data));
        std::cout << "Sent ciphertext to server" << std::endl;

        // Serialize public key
        std::stringstream ss_pk;
        Serial::Serialize(keys.publicKey, ss_pk, SerType::BINARY);
        std::string pk_data = ss_pk.str();
        data_size = pk_data.size();

        // Send the serialized public key
        boost::asio::write(socket, boost::asio::buffer(&data_size, sizeof(data_size)));
        boost::asio::write(socket, boost::asio::buffer(pk_data));
        std::cout << "Sent public key to server" << std::endl;

        // Receive the size of the incoming data
        boost::asio::read(socket, boost::asio::buffer(&data_size, sizeof(data_size)));

        // Receive the processed data from the server
        std::vector<char> received_data(data_size);
        boost::asio::read(socket, boost::asio::buffer(received_data));
        std::cout << "Received processed data from server" << std::endl;

        // Deserialize the received data
        std::stringstream ss_received(std::string(received_data.data(), received_data.size()));
        Ciphertext<DCRTPoly> received_ciphertext;
        Serial::Deserialize(received_ciphertext, ss_received, SerType::BINARY);
        std::cout << "Deserialized processed data" << std::endl;

        // Decrypt the received data
        Plaintext received_plaintext;
        cryptoContext->Decrypt(keys.secretKey, received_ciphertext, &received_plaintext);
        received_plaintext->SetLength(2);

        // Output the decrypted coordinates
        auto received_coords = received_plaintext->GetPackedValue();
        double latitude_diff = static_cast<double>(received_coords[0]) / 100;
        double longitude_diff = static_cast<double>(received_coords[1]) / 100;
        std::cout << "Latitude Difference: " << latitude_diff << std::endl;
        std::cout << "Longitude Difference: " << longitude_diff << std::endl;

        // Calculate distance in meters
        double distance = haversine(latitude, longitude, latitude + latitude_diff, longitude + longitude_diff);
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Distance Difference: " << distance << " meters" << std::endl;

        // Send message to server if distance is within 1000 meters
        std::string distance_message = (distance < 1000) ? "1000미터 이내입니다." : "1000미터 이상입니다.";
        boost::asio::write(socket, boost::asio::buffer(distance_message + "\n"));

        if (distance < 1000) {
            std::cout << "1000미터 이내입니다." << std::endl;
        }

        // Wait for some time before closing the connection
        std::this_thread::sleep_for(std::chrono::seconds(1));

    } catch (std::exception& e) {
        std::cerr << "Exception in victim client: " << e.what() << std::endl;
    }

    return 0;
}
