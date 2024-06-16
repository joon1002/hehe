#include "openfhe.h"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>
#include <vector>

using namespace lbcrypto;
using boost::asio::ip::tcp;

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

        // Receive public key from server
        uint32_t pk_data_size;
        boost::asio::read(socket, boost::asio::buffer(&pk_data_size, sizeof(pk_data_size)));
        std::cout << "Received public key size: " << pk_data_size << std::endl;

        std::vector<char> pk_data(pk_data_size);
        boost::asio::read(socket, boost::asio::buffer(pk_data));
        std::cout << "Received public key data." << std::endl;

        std::stringstream ss_pk(std::string(pk_data.data(), pk_data.size()));
        PublicKey<DCRTPoly> publicKey;
        Serial::Deserialize(publicKey, ss_pk, SerType::BINARY);
        std::cout << "Deserialized public key." << std::endl;

        // Input latitude and longitude
        double latitude, longitude;
        std::cout << "Enter latitude: ";
        std::cin >> latitude;
        std::cout << "Enter longitude: ";
        std::cin >> longitude;

        // Adjust and scale coordinates
        double adjustedLatitude = latitude - 36.0;
        double adjustedLongitude = longitude - 125.0;
        int64_t intLatitude = static_cast<int64_t>(adjustedLatitude * 10000); // scaling to 4 decimal places
        int64_t intLongitude = static_cast<int64_t>(adjustedLongitude * 10000); // scaling to 4 decimal places

        // Encrypt data
        std::vector<int64_t> coordinates = {intLatitude, intLongitude};
        Plaintext plaintext = cryptoContext->MakePackedPlaintext(coordinates);
        auto ciphertext = cryptoContext->Encrypt(publicKey, plaintext);
        std::cout << "Encrypted coordinates." << std::endl;

        // Serialize ciphertext
        std::stringstream ss;
        Serial::Serialize(ciphertext, ss, SerType::BINARY);
        std::string data = ss.str();
        uint32_t data_size = data.size();
        std::cout << "Serialized ciphertext, size: " << data_size << std::endl;

        // Send the serialized ciphertext to the server
        boost::asio::write(socket, boost::asio::buffer(&data_size, sizeof(data_size)));
        boost::asio::write(socket, boost::asio::buffer(data));
        std::cout << "Sent ciphertext to server." << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
