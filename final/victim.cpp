#include "openfhe.h"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>
#include <vector>
#include <cmath>

using namespace lbcrypto;
using boost::asio::ip::tcp;

double haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000; // Earth radius in meters
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;

    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;

    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1) * cos(lat2) * 
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    double distance = R * c;

    return distance;
}

int main() {
    try {
        // Initialize Boost ASIO for network communication
        boost::asio::io_context io_context;

        // Server address and port
        std::string server_address = "34.125.140.55"; // Change to your server's IP address
        std::string server_port = "33333"; // Change to your server's listening port

        // Try to connect to the server
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server_address, server_port);
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        std::cout << "Connected to server." << std::endl;

        // CryptoContext setup
        CCParams<CryptoContextBFVRNS> parameters;
        parameters.SetPlaintextModulus(65537);
        parameters.SetSecurityLevel(HEStd_128_classic);
        parameters.SetRingDim(8192);
        parameters.SetMultiplicativeDepth(1);

        CryptoContext<DCRTPoly> cryptoContext = GenCryptoContext(parameters);
        cryptoContext->Enable(PKE);
        cryptoContext->Enable(KEYSWITCH);
        cryptoContext->Enable(LEVELEDSHE);
        cryptoContext->Enable(MULTIPARTY);

        // Key Generation (for decryption)
        auto keys = cryptoContext->KeyGen();
        cryptoContext->EvalMultKeyGen(keys.secretKey);
        std::cout << "Generated public and private keys." << std::endl;

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
        std::cout << "Encrypted coordinates." << std::endl;

        // Serialize public key
        std::stringstream ss_pk;
        Serial::Serialize(keys.publicKey, ss_pk, SerType::BINARY);
        std::string pk_data = ss_pk.str();
        uint32_t pk_data_size = pk_data.size();
        std::cout << "Serialized public key, size: " << pk_data_size << std::endl;

        // Serialize ciphertext
        std::stringstream ss;
        Serial::Serialize(ciphertext, ss, SerType::BINARY);
        std::string ct_data = ss.str();
        uint32_t ct_data_size = ct_data.size();
        std::cout << "Serialized ciphertext, size: " << ct_data_size << std::endl;

        // Send the public key and serialized ciphertext to the server
        boost::asio::write(socket, boost::asio::buffer(&pk_data_size, sizeof(pk_data_size)));
        boost::asio::write(socket, boost::asio::buffer(pk_data));
        boost::asio::write(socket, boost::asio::buffer(&ct_data_size, sizeof(ct_data_size)));
        boost::asio::write(socket, boost::asio::buffer(ct_data));
        std::cout << "Sent public key and ciphertext to server." << std::endl;

        // Receive the size of the incoming data
        boost::asio::read(socket, boost::asio::buffer(&ct_data_size, sizeof(ct_data_size)));
        std::cout << "Received processed data size from server: " << ct_data_size << std::endl;

        // Receive the processed data from the server
        std::vector<char> received_data(ct_data_size);
        boost::asio::read(socket, boost::asio::buffer(received_data));
        std::cout << "Received processed data from server." << std::endl;

        // Deserialize the received data
        std::stringstream ss_received(std::string(received_data.data(), received_data.size()));
        Ciphertext<DCRTPoly> received_ciphertext;
        Serial::Deserialize(received_ciphertext, ss_received, SerType::BINARY);
        std::cout << "Deserialized processed ciphertext." << std::endl;

        // Decrypt the result
        Plaintext plaintext_result;
        cryptoContext->Decrypt(keys.secretKey, received_ciphertext, &plaintext_result);
        plaintext_result->SetLength(2);
        std::cout << "Decrypted final result." << std::endl;

        // Output the decrypted coordinates
        auto received_coords = plaintext_result->GetPackedValue();
        double latitude_diff = static_cast<double>(received_coords[0]) / 100;
        double longitude_diff = static_cast<double>(received_coords[1]) / 100;
        std::cout << "Final Processed Latitude Difference: " << latitude_diff << std::endl;
        std::cout << "Final Processed Longitude Difference: " << longitude_diff << std::endl;

        // Calculate distance using Haversine formula
        double distance = haversine(latitude, longitude, latitude + latitude_diff, longitude + longitude_diff);
        std::cout << "Distance: " << distance << " meters" << std::endl;

        if (distance < 1000) {
            std::cout << "가해자가 접근제한 거리를 위반하였습니다" << std::endl;
        }

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
