#include "openfhe.h"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>

using namespace lbcrypto;
using boost::asio::ip::tcp;

void handle_client(tcp::socket& socket, CryptoContext<DCRTPoly>& cryptoContext, std::pair<int64_t, int64_t>& attacker_coords, Ciphertext<DCRTPoly>& victim_ciphertext, PublicKey<DCRTPoly>& victim_publicKey, bool& victim_received) {
    try {
        // Receive the identifier
        char client_type;
        boost::asio::read(socket, boost::asio::buffer(&client_type, sizeof(client_type)));

        if (client_type == 'A') {
            // Attacker client
            std::cout << "Attacker client connected." << std::endl;

            // Receive the size of the incoming data
            uint32_t data_size;
            boost::asio::read(socket, boost::asio::buffer(&data_size, sizeof(data_size)));

            // Receive data
            std::vector<char> data(data_size);
            boost::asio::read(socket, boost::asio::buffer(data));

            // Deserialize attacker coordinates (plain text)
            std::stringstream ss(std::string(data.data(), data.size()));
            ss >> attacker_coords.first >> attacker_coords.second;
            std::cout << "Received attacker coordinates: " << attacker_coords.first << ", " << attacker_coords.second << std::endl;

        } else if (client_type == 'V') {
            // Victim client
            std::cout << "Victim client connected." << std::endl;

            // Receive the size of the incoming data
            uint32_t data_size;
            boost::asio::read(socket, boost::asio::buffer(&data_size, sizeof(data_size)));

            // Receive data
            std::vector<char> data(data_size);
            boost::asio::read(socket, boost::asio::buffer(data));

            // Deserialize victim ciphertext
            std::stringstream ss(std::string(data.data(), data.size()));
            Serial::Deserialize(victim_ciphertext, ss, SerType::BINARY);
            std::cout << "Received victim ciphertext" << std::endl;

            // Deserialize victim public key
            boost::asio::read(socket, boost::asio::buffer(&data_size, sizeof(data_size)));
            data.resize(data_size);
            boost::asio::read(socket, boost::asio::buffer(data));
            std::stringstream ss_pk(std::string(data.data(), data.size()));
            Serial::Deserialize(victim_publicKey, ss_pk, SerType::BINARY);
            std::cout << "Received victim public key" << std::endl;
            
            victim_received = true;
        }

    } catch (std::exception& e) {
        std::cerr << "Exception in handling client: " << e.what() << std::endl;
    }
}

int main() {
    // Initialize Boost ASIO for network communication
    boost::asio::io_context io_context;

    // CryptoContext setup
    CCParams<CryptoContextBFVRNS> parameters;
    parameters.SetPlaintextModulus(65537);
    parameters.SetMultiplicativeDepth(1);
    parameters.SetRingDim(8192);
    parameters.SetSecurityLevel(HEStd_128_classic);

    CryptoContext<DCRTPoly> cryptoContext = GenCryptoContext(parameters);
    cryptoContext->Enable(PKE);
    cryptoContext->Enable(KEYSWITCH);
    cryptoContext->Enable(LEVELEDSHE);

    // Create a TCP server socket
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 33333));
    std::cout << "Server listening on port 33333..." << std::endl;

    std::pair<int64_t, int64_t> attacker_coords;
    Ciphertext<DCRTPoly> victim_ciphertext;
    PublicKey<DCRTPoly> victim_publicKey;
    bool victim_received = false;

    // Wait for connections
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Client connected." << std::endl;

        handle_client(socket, cryptoContext, attacker_coords, victim_ciphertext, victim_publicKey, victim_received);

        if (victim_received) {
            // Perform homomorphic subtraction using attacker's coordinates
            Plaintext attacker_plaintext = cryptoContext->MakePackedPlaintext({-attacker_coords.first, -attacker_coords.second});
            auto attacker_ciphertext = cryptoContext->Encrypt(victim_publicKey, attacker_plaintext);
            auto result_ciphertext = cryptoContext->EvalAdd(victim_ciphertext, attacker_ciphertext);

            // Serialize the result
            std::stringstream ss_result;
            Serial::Serialize(result_ciphertext, ss_result, SerType::BINARY);
            std::string result_data = ss_result.str();
            uint32_t result_data_size = result_data.size();

            // Send the size of the result data
            boost::asio::write(socket, boost::asio::buffer(&result_data_size, sizeof(result_data_size)));

            // Send the result data
            boost::asio::write(socket, boost::asio::buffer(result_data));
            std::cout << "Sent result to victim" << std::endl;

            // Check for distance message from victim client
            boost::system::error_code error;
            boost::asio::streambuf buffer;
            boost::asio::read(socket, buffer, error);
            if (!error || error == boost::asio::error::eof) {
                std::istream is(&buffer);
                std::string distance_message;
                std::getline(is, distance_message);
                if (distance_message == "1000미터 이내입니다.") {
                    std::cout << "접근제한 거리 이내에 피해자가 있습니다" << std::endl;
                }
            }

            // Reset for next client pair
            victim_received = false;
        }
    }

    return 0;
}
