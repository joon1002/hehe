// server.cpp
#include "openfhe.h"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>

using namespace lbcrypto;
using boost::asio::ip::tcp;

void handle_client(tcp::socket& socket, CryptoContext<DCRTPoly>& cryptoContext) {
    try {
        // Receive the size of the incoming data
        uint32_t data_size;
        boost::asio::read(socket, boost::asio::buffer(&data_size, sizeof(data_size)));

        // Receive data
        std::vector<char> data(data_size);
        boost::asio::read(socket, boost::asio::buffer(data));

        // Deserialize ciphertext
        std::stringstream ss(std::string(data.data(), data.size()));
        Ciphertext<DCRTPoly> ciphertext;
        Serial::Deserialize(ciphertext, ss, SerType::BINARY);

        // Deserialize public key
        boost::asio::read(socket, boost::asio::buffer(&data_size, sizeof(data_size)));
        data.resize(data_size);
        boost::asio::read(socket, boost::asio::buffer(data));
        std::stringstream ss_pk(std::string(data.data(), data.size()));
        PublicKey<DCRTPoly> publicKey;
        Serial::Deserialize(publicKey, ss_pk, SerType::BINARY);

        // Perform homomorphic addition (add 100 to latitude and longitude to simulate adding 1)
        Plaintext one = cryptoContext->MakePackedPlaintext({100, 100});
        auto ciphertext_one = cryptoContext->Encrypt(publicKey, one);
        auto result_ciphertext = cryptoContext->EvalAdd(ciphertext, ciphertext_one);

        // Serialize the result
        std::stringstream ss_result;
        Serial::Serialize(result_ciphertext, ss_result, SerType::BINARY);
        std::string result_data = ss_result.str();
        uint32_t result_data_size = result_data.size();

        // Send the size of the result data
        boost::asio::write(socket, boost::asio::buffer(&result_data_size, sizeof(result_data_size)));

        // Send the result data
        boost::asio::write(socket, boost::asio::buffer(result_data));

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

    // Wait for a connection
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Client connected." << std::endl;

        handle_client(socket, cryptoContext);
    }

    return 0;
}
