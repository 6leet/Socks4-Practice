#include <iostream>
#include <sstream>
#include <unistd.h>
#include "firewall.hpp"
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

firewall firewall_("socks.conf");

class session : public enable_shared_from_this<session> {
public:
    session(boost::asio::io_context& io_context, tcp::socket socket) : 
        io_context_(io_context), csocket_(move(socket)), ssocket_(io_context), resolver_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), 0)) {}
    void start() {
        src_ip = csocket_.remote_endpoint().address().to_string();
        src_port = csocket_.remote_endpoint().port();
        firewall_.update_config();
        read_request();
        request_handler();
    }
private:
    void request_parser(int length) {
        version = data_[0];
        command_code = data_[1];
        dst_port = data_[2] * 256 + data_[3];
        dst_ip = to_string(data_[4]) + "." + to_string(data_[5]) + "." + to_string(data_[6]) + "." + to_string(data_[7]);
        dst_ip_bytes[0] = data_[4], dst_ip_bytes[1] = data_[5], dst_ip_bytes[2] = data_[6], dst_ip_bytes[3] = data_[7];
        int i;
        for (i = 8; i < length; i++) {
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
        cout << "version: " << version << '\n' << flush;
        cout << "command_code: " << command_code << '\n' << flush;
        cout << "dst_port: " << dst_port << '\n' << flush;
        cout << "dst_ip: " << dst_ip << '\n' << flush;
        cout << "user_id: " << user_id << '\n' << flush;
        cout << "domain_name: " << domain_name << '\n' << flush;
    }

    void do_read(int direction) {
        auto self(shared_from_this());
        if (direction & 0x01) { // client -> server
            csocket_.async_read_some(boost::asio::buffer(buf1_, buf_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    buf1_[length] = '\0';
                    do_write(1, length);
                }
            });
        }
        if (direction & 0x02) { // server -> client
            ssocket_.async_read_some(boost::asio::buffer(buf2_, buf_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    buf2_[length] = '\0';
                    do_write(2, length);
                }
            });
        }
    }

    void do_write(int direction, int length) {
        auto self(shared_from_this());
        if (direction & 0x01) { // client -> server
            boost::asio::async_write(ssocket_, boost::asio::buffer(buf1_, length), [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read(1);
                }
            });
        }
        if (direction & 0x02) { // server -> client
            boost::asio::async_write(csocket_, boost::asio::buffer(buf2_, length), [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read(2);
                }
            });
        }
    }

    void bind_read(int direction) {
        auto self(shared_from_this());
        if (direction & 0x01) { // client -> server
            csocket_.async_read_some(boost::asio::buffer(buf1_, buf_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    buf1_[length] = '\0';
                    bind_write(1, length);
                } else {
                    ssocket_.close();
                    csocket_.close();
                }
            });
        }
        if (direction & 0x02) { // server -> client
            ssocket_.async_read_some(boost::asio::buffer(buf2_, buf_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    buf2_[length] = '\0';
                    bind_write(2, length);
                } else {
                    ssocket_.close();
                    csocket_.close();
                }
            });
        }
    }

    void bind_write(int direction, int length) {
        auto self(shared_from_this());
        if (direction & 0x01) { // client -> server
            boost::asio::async_write(ssocket_, boost::asio::buffer(buf1_, length), [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    bind_read(1);
                }
            });
        }
        if (direction & 0x02) { // server -> client
            boost::asio::async_write(csocket_, boost::asio::buffer(buf2_, length), [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    bind_read(2);
                }
            });
        }
    }

    void do_accept() {
        auto self(shared_from_this());
        acceptor_.async_accept([this, self](boost::system::error_code ec, tcp::socket socket_) {
            if (!ec) {
                // ssocket_ = socket;
                ssocket_.assign(tcp::v4(), socket_.release());
                string _dst_ip = ssocket_.remote_endpoint().address().to_string();
                int _dst_port = ssocket_.remote_endpoint().port();
                if (dst_ip != _dst_ip) {
                    console("BIND", "Reject");
                    reply(0, 91, dst_port, dst_ip_bytes);
                    ssocket_.close();
                    csocket_.close();
                } else {
                    console("BIND", "Accept");
                    reply(0, 90, dst_port, dst_ip_bytes);
                    bind_read(3); // ?
                }
            }
        });
    }

    bool resolve_and_connect() {
        boost::system::error_code ec;
        boost::asio::ip::tcp::resolver::results_type results = resolver_.resolve(domain_name, to_string(dst_port), ec);
        if (!ec) {
            for (auto it = results.begin(); it != results.end(); it++) {
                boost::system::error_code ec;
                ssocket_.connect((*it).endpoint(), ec);
                if (!ec) {
                    dst_ip = (*it).endpoint().address().to_string();
                    return true;
                }
            }
            return false;
        }
    }

    void console(string command, string reply) {
        cout << "<S_IP>: " << src_ip << '\n';
        cout << "<S_PORT>: " << src_port << '\n';
        cout << "<D_IP>: " << dst_ip << '\n';
        cout << "<D_PORT>: " << dst_port << '\n';
        cout << "<Command>: " << command << '\n';
        cout << "<Reply>: " << reply << '\n';
    }

    void reply(int version, int status_code, int port, int dst_ip_bytes[4]) {
        unsigned char reply_[8];
        memset(reply_, 0, sizeof(reply_));
        reply_[0] = version;
        reply_[1] = status_code;
        reply_[2] = port / 256, reply_[3] = port % 256;
        reply_[4] = dst_ip_bytes[0], reply_[5] = dst_ip_bytes[1], reply_[6] = dst_ip_bytes[2], reply_[7] = dst_ip_bytes[3];
        size_t length = csocket_.send(boost::asio::buffer(reply_, 8));
    }

    void connect() {
        if (!firewall_.check_valid(dst_ip, 'c')) {
            console("CONNECT", "Reject");
            reply(0, 91, dst_port, dst_ip_bytes);
            return;
        }
        if (domain_name == "") { // wrong condition
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(dst_ip), dst_port);
            boost::system::error_code ec;
            ssocket_.connect(endpoint, ec);
            if (!ec) {
                console("CONNECT", "Accept");
                reply(0, 90, dst_port, dst_ip_bytes);
                do_read(3);
            } else {
                console("CONNECT", "Reject");
                reply(0, 91, dst_port, dst_ip_bytes);
            }
        } else {
            if (resolve_and_connect()) {
                console("CONNECT", "Accept");
                reply(0, 90, dst_port, dst_ip_bytes);
                do_read(3);
            } else {
                console("CONNECT", "Reject");
                reply(0, 91, dst_port, dst_ip_bytes);
            }
        }
    }

    void _bind() {
        if (!firewall_.check_valid(dst_ip, 'b')) {
            console("BIND", "Reject");
            reply(0, 91, dst_port, dst_ip_bytes);
            return;
        }
        socks_port = acceptor_.local_endpoint().port();
        memset(socks_ip_bytes, 0, sizeof(socks_ip_bytes));
        // console("BIND", "Accept");
        reply(0, 90, socks_port, socks_ip_bytes);
        do_accept();
        return;
    }

    void request_handler() {
        if (version != 0x04) {
            reply(0, 91, dst_port, dst_ip_bytes);
            return;
        }
        if (command_code == 0x01) {
            connect();
        } else if (command_code == 0x02) {
            _bind();
        } else {
            reply(0, 91, dst_port, dst_ip_bytes);
            return;
        }
    }

    void read_request() {
        auto self(shared_from_this()); // make `this` session outlives the asynchronous operation
        size_t length = csocket_.receive(boost::asio::buffer(data_, max_length));
        request_parser(length);
        // _debug();
    }

    /* boost asio objects */
    boost::asio::io_context& io_context_;
    tcp::socket csocket_;
    tcp::socket ssocket_;
    tcp::resolver resolver_;
    tcp::acceptor acceptor_;

    /* buffers */
    enum { max_length = 1024, buf_length = 10000};
    unsigned char data_[max_length];
    char buf1_[buf_length], buf2_[buf_length];

    /* socks4/4a request */
    int version;
    int command_code;
    int src_port;
    string src_ip;
    int dst_port;
    int dst_ip_bytes[4];
    string dst_ip;
    int user_id;
    string domain_name;
    int socks_port;
    int socks_ip_bytes[4];
};


class server {
public:
    server(boost::asio::io_context& io_context, short port) : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }
private:
    void do_accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                io_context_.notify_fork(boost::asio::io_context::fork_prepare);

                pid_t pid = fork();
                if (pid < 0) {
                    cerr << "fork error.\n" << flush;
                } else if (pid == 0) {
                    io_context_.notify_fork(boost::asio::io_context::fork_child);

                    acceptor_.close();
                    make_shared<session>(io_context_, move(socket))->start();
                    return;
                } else {
                    io_context_.notify_fork(boost::asio::io_context::fork_parent);

                    socket.close();
                }
                // make_shared<session>(move(socket))->start();
            }
            do_accept();
        });
    }
    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            cerr << "Usage: http_server <port>\n";
            return 1;
        }
        boost::asio::io_context io_context;
        server s(io_context, atoi(argv[1]));
        io_context.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << '\n';
    }
}
