#include <iostream>

using namespace std;

void request_parser(int length) {
    cout << "in request_parser.\n" << flush;
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