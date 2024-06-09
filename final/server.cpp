#include "openfhe.h"
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>
#include <vector>

using namespace lbcrypto;
using boost::asio::ip::tcp;

void handle_client(tcp::socket& victim_socket, tcp::socket& perpetrator_socket, CryptoContext<DCRTPoly>& cryptoContext) {
    try {
        std::cout << "Receiving public key size from victim client..." << std::endl;
        uint32_t pk_size;
        boost::asio::read(victim_socket, boost::asio::buffer(&pk_size, sizeof(pk_size)));

        std::cout << "Receiving public key from victim client..." << std::endl;
        std::vector<char> pk_data(pk_size);
        boost::asio::read(victim_socket, boost::asio::buffer(pk_data));

        std::cout << "Deserializing public key from victim client..." << std::endl;
        std::stringstream ss_pk(std::string(pk_data.data(), pk_data.size()));
        PublicKey<DCRTPoly> publicKey;
        Serial::Deserialize(publicKey, ss_pk, SerType::BINARY);

        std::cout << "Receiving ciphertext size from victim client..." << std::endl;
        uint32_t ct_size_victim;
        boost::asio::read(victim_socket, boost::asio::buffer(&ct_size_victim, sizeof(ct_size_victim)));

        std::cout << "Receiving ciphertext from victim client..." << std::endl;
        std::vector<char> ct_data_victim(ct_size_victim);
        boost::asio::read(victim_socket, boost::asio::buffer(ct_data_victim));

        std::cout << "Deserializing ciphertext from victim client..." << std::endl;
        std::stringstream ss_ct_victim(std::string(ct_data_victim.data(), ct_data_victim.size()));
        Ciphertext<DCRTPoly> ct_victim;
        Serial::Deserialize(ct_victim, ss_ct_victim, SerType::BINARY);

        std::cout << "Sending public key to perpetrator client..." << std::endl;
        boost::asio::write(perpetrator_socket, boost::asio::buffer(&pk_size, sizeof(pk_size)));
        boost::asio::write(perpetrator_socket, boost::asio::buffer(pk_data));

        std::cout << "Receiving ciphertext size from perpetrator client..." << std::endl;
        uint32_t ct_size_perpetrator;
        boost::asio::read(perpetrator_socket, boost::asio::buffer(&ct_size_perpetrator, sizeof(ct_size_perpetrator)));

        std::cout << "Receiving ciphertext from perpetrator client..." << std::endl;
        std::vector<char> ct_data_perpetrator(ct_size_perpetrator);
        boost::asio::read(perpetrator_socket, boost::asio::buffer(ct_data_perpetrator));

        std::cout << "Deserializing ciphertext from perpetrator client..." << std::endl;
        std::stringstream ss_ct_perpetrator(std::string(ct_data_perpetrator.data(), ct_data_perpetrator.size()));
        Ciphertext<DCRTPoly> ct_perpetrator;
        Serial::Deserialize(ct_perpetrator, ss_ct_perpetrator, SerType::BINARY);

        std::cout << "Performing homomorphic subtraction..." << std::endl;
        auto result_ciphertext = cryptoContext->EvalSub(ct_victim, ct_perpetrator);

        std::cout << "Serializing the result..." << std::endl;
        std::stringstream ss_result;
        Serial::Serialize(result_ciphertext, ss_result, SerType::BINARY);
        std::string result_data = ss_result.str();
        uint32_t result_data_size = result_data.size();

        std::cout << "Sending result data size to victim client..." << std::endl;
        boost::asio::write(victim_socket, boost::asio::buffer(&result_data_size, sizeof(result_data_size)));
        boost::asio::write(victim_socket, boost::asio::buffer(result_data));

    } catch (std::exception& e) {
        std::cerr << "Exception in handling client: " << e.what() << std::endl;
    }
}

int main() {
    try {
        boost::asio::io_context io_context;

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

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 33333));
        std::cout << "Server listening on port 33333..." << std::endl;

        while (true) {
            tcp::socket victim_socket(io_context);
            acceptor.accept(victim_socket);
            std::cout << "Victim client connected." << std::endl;

            tcp::socket perpetrator_socket(io_context);
            acceptor.accept(perpetrator_socket);
            std::cout << "Perpetrator client connected." << std::endl;

            handle_client(victim_socket, perpetrator_socket, cryptoContext);
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
