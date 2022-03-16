#include <iostream>
#include <unistd.h>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class session : public enable_shared_from_this<session> {
public:
    session(tcp::socket socket) : csocket_(move(socket)), ssocket_(csocket_.get_executor()) {
        // server_addr = socket_.local_endpoint().address().to_string();
        // server_port = socket_.local_endpoint().port();
        // remote_addr = socket_.remote_endpoint().address().to_string();
        // remote_port = socket_.remote_endpoint().port();
        // boost::asio::io_context io_context;
    }
    void start() {
        cerr << "in session.\n" << flush;
        cerr << csocket_.native_handle() << '\n';
        read_request();
    }
private:
    void request_parser(int length) {
        cerr << "in request_parser.\n" << flush;
        version = data_[0];
        command_code = data_[1];
        dst_port = data_[2] * 256 + data_[3];
        dst_ip = to_string(data_[4]) + ":" + to_string(data_[5]) + ":" + to_string(data_[6]) + ":" + to_string(data_[7]);
        int i;
        for (i = 8; i < length; i++) { // last byte is NULL byte
            if (data_[i] == 0) {
                break;
            }
            user_id = user_id * 256 + data_[i];
        }
        i += 1;
        for ( ; i < length; i++) {
            if (data_[i] == 0) {
                break;
            }
            domain_name = domain_name + char(data_[i]);
        }
    }
    void _debug() {
        cerr << "version: " << version << '\n' << flush;
        cerr << "command_code: " << command_code << '\n' << flush;
        cerr << "dst_port: " << dst_port << '\n' << flush;
        cerr << "dst_ip: " << dst_ip << '\n' << flush;
        cerr << "user_id: " << user_id << '\n' << flush;
        cerr << "domain_name: " << domain_name << '\n' << flush;
    }
    void read_request() {
        cerr << "in request.\n";
        auto self(shared_from_this()); // make `this` session outlives the asynchronous operation
        csocket_.async_read_some(boost::asio::buffer(data_, max_length), [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                cerr << "shit not again.\n" << flush;
                request_parser(length);
                _debug();
			}
        });
    }

    tcp::socket csocket_;
    tcp::socket ssocket_;
    unsigned short server_port, remote_port;
    enum { max_length = 1024 };
    unsigned char data_[max_length];
    int version;
    int command_code;
    int dst_port;
    string dst_ip;
    int user_id;
    string domain_name;
};

int main() {
    boost::asio::io_context io_context;
    session s(io_context);
    io_context.run();
}