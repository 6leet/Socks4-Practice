#ifndef FIREWALL
#define FIREWALL

#include <string>
#include <vector>
#include <regex>

using namespace std;

struct rule {
    char type;
    regex reg;
};

struct firewall {
    string filename;
    vector<rule> rules;
    firewall(string);
    void update_config();
    string convert_regex(string);
    bool check_valid(string, char);
};

#endif