#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <boost/asio.hpp>
#include "html_handler.hpp"

using namespace std;
using boost::asio::ip::tcp;

string socks_server;
int socks_port;

class client : public enable_shared_from_this<client>{
public:
    client(shared_ptr<tcp::socket> socket, serverInfo _sinfo) : socket_(socket) {
        sinfo = _sinfo;
        cerr << "in client. socket: " << socket_->native_handle() << '\n' << flush;
        do_readfile();
        id = 0;
        session_id = "s" + to_string(sinfo.id);
    }
    void start() {
        do_request();
    }
private:
    void do_request() {
        vector<unsigned char> connect_request;
        connect_request.push_back(4);
        connect_request.push_back(1);
        connect_request.push_back(stoi(sinfo.port) / 256);
        connect_request.push_back(stoi(sinfo.port) % 256);
        for (int i = 0; i < 3; i++) connect_request.push_back(0);
        connect_request.push_back(1);
        connect_request.push_back(0);
        for (int i = 0; i < sinfo.host.size(); i++) {
            connect_request.push_back(sinfo.host[i]);
        }
        connect_request.push_back(0);

        auto self(shared_from_this());
        boost::asio::async_write(*socket_, boost::asio::buffer(connect_request.data(), connect_request.size()), [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // cerr << "do_write()\n" <<  flush;
                get_reply();
            } else {
                cerr << "socket: " << socket_->native_handle() << '\n';
            }
        });
    }
    void get_reply() {
        unsigned char reply[8];
        memset(reply, 0, sizeof(reply));
        auto self(shared_from_this());
        boost::asio::async_read(*socket_, boost::asio::buffer(reply, 8), boost::asio::transfer_at_least(8), [this, self, reply](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                // output_command(session_id, to_string(length));
                do_read();
            } else {
                get_reply();
            }
        });
    }
    vector<string> do_readfile() {
        reqs.clear();

        char buf[15005];
        ifstream stream("test_case/" + sinfo.filename);
        if (!stream.is_open()) {
            return reqs;
        }
        
        while (stream.getline(buf, 15005, '\n')) {
            string req(buf);
            reqs.push_back(req);
        }
        stream.close();
        return reqs;
    }
    void do_read() {
        string req = reqs[id] + '\n';
        auto self(shared_from_this());
        socket_->async_read_some(boost::asio::buffer(data_, max_length), [this, self, req](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                string data(data_);
                // cerr << "receive: " << data << "<endhere>\n" << flush;
                memset(data_, 0, sizeof(data_));

                bool prompt_flag = false;
                if (data.find("% ") != string::npos) {
                    prompt_flag = true;
                }
                output_shell(session_id, data);
                if (!prompt_flag) {
                    do_read();
                } else {
                    strcpy(req_, req.c_str());
                    output_command(session_id, req);
                    do_write(req.size());
                    id++;
                }
            }
        });
    }
    void do_write(std::size_t length) {
        // cerr << req_ << ' ' << length << '\n' << flush;
        bool exit_ = false;
        string req(req_);
        if (req.find("exit") != string::npos) {
            exit_ = true;
        }
        auto self(shared_from_this());
        boost::asio::async_write(*socket_, boost::asio::buffer(req_, length), [this, self, exit_](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // cerr << "do_write()\n" <<  flush;
                if (exit_) {
                    cerr << "socket: " << socket_->native_handle() << '\n' << flush;
                    socket_->close();
                    return;
                }
                do_read();
            } else {
                cerr << "socket: " << socket_->native_handle() << '\n';
            }
        });
    }
    void _debug() {
        cerr << "debugger.\n" << flush;
        cerr << "socket: " << socket_->native_handle() << '\n';
    }
    shared_ptr<tcp::socket> socket_; // socks server socket
    serverInfo sinfo;
    vector<string> reqs;
    int id;
    enum { max_length = 1024 };
    char data_[max_length];
    char req_[max_length];
    string session_id;
};

class controller {
public:
    controller(boost::asio::io_context& io_context, vector<serverInfo> &_sinfos) : resolver_(io_context) {
        sinfos = _sinfos;
        do_connect(io_context);
    }
private:
    void do_connect(boost::asio::io_context& io_context) {
        for (int id = 0; id < sinfos.size(); id++) {
            serverInfo sinfo = sinfos[id];
            if (sinfo.host == "") {
                continue;
            }
            shared_ptr<tcp::socket> socket_ = make_shared<tcp::socket>(io_context);
            resolver_.async_resolve(socks_server, to_string(socks_port), [this, socket_, sinfo](boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
                if (!ec) {
                    for (auto it = results.begin(); it != results.end(); it++) {
                        socket_->async_connect((*it).endpoint(), [socket_, sinfo](const boost::system::error_code &ec_) {
                            if (!ec_) {
                                make_shared<client>(move(socket_), sinfo)->start();
                            } 
                        });
                    }
                }
            });
        }
    }
    tcp::resolver resolver_;
    boost::asio::io_context io_context_;
    vector<serverInfo> sinfos;
};

vector<serverInfo> parse_query_string() {
    vector<serverInfo> sinfo;

    char* buf = getenv("QUERY_STRING");
    if (buf == NULL) {
        return sinfo;
    }
    string query_string(buf), query;
    istringstream ss(query_string);
    sinfo.resize(5);

    int qcnt = 0;
    while (getline(ss, query, '&')) {
        int id = qcnt / 3;
        string value = "";

        int pos = query.find('=');
        if (pos != string::npos) {
            value = query.substr(pos + 1, query.size() - pos - 1);
        } 

        if (id == 5) {
            if (qcnt % 3 == 0) { // sock server hostname
                socks_server = value;
            } else if (qcnt % 3 == 1) { // sock server port
                socks_port = stoi(value);
            }
            qcnt++;
            continue;
        }

        if (qcnt % 3 == 0) {
            sinfo[id].host = value;
            sinfo[id].id = id;
        } else if (qcnt % 3 == 1) {
            sinfo[id].port = value;
        } else {
            sinfo[id].filename = value;
        }
        qcnt++;
    }
    return sinfo;
}

int main() {
    vector<serverInfo> sinfo = parse_query_string();
    console_format(sinfo);
    // output_command("s0", socks_server + ':' + to_string(socks_port));

    // cerr << socks_server << ':' << socks_port << '\n';

    boost::asio::io_context io_context;
    controller c(io_context, sinfo);
    io_context.run();
}
