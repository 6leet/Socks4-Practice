#include <regex>
#include <string>
#include <fstream>
#include <iostream>
#include "firewall.hpp"

using namespace std;

firewall::firewall(string _filename) : filename(_filename) {
    update_config();
}

void firewall::update_config() {
    rules.clear();
    
    char buf[1024];
    ifstream stream(filename);
    if (!stream.is_open()) {
        return;
    }

    while (stream.getline(buf, 1024, '\n')) {
        rule _rule;
        string s;
        stringstream ss(buf);
        int i = 0;
        // cout << buf << '\n';
        while (ss >> s) {
            if (i == 1) {
                _rule.type = s[0];
            } else if (i == 2) {
                regex reg(convert_regex(s));
                _rule.reg = reg;
            } else if (i > 2) {
                break;
            }
            i++;
        }
        rules.push_back(_rule);
    }
    stream.close();
}

string firewall::convert_regex(string s) {
    int cur_i = -1, esc_i = s.find("*");
    while (esc_i != string::npos && esc_i > cur_i) {
        cur_i = esc_i;
        s.replace(cur_i, 1, "[0-9]+");
        esc_i = s.find("*");
    }
    // cout << s << '\n';
    return s;
}

bool firewall::check_valid(string ip, char type) {
    bool isvalid = false;
    for (int i = 0; i < rules.size(); i++) {
        // cout << rules[i].type << '\n';
        // cout << regex_match(ip, rules[i].reg) << '\n';
        if (rules[i].type == type && regex_match(ip, rules[i].reg)) {
            isvalid = true;
            break;
        }
    }
    return isvalid;
}