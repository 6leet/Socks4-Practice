#ifndef HTML_HANDLER
#define HTML_HANDLER

#include <string>
#include <vector>

using namespace std;

struct serverInfo {
    int id = -1;
    string host = "";
    string port = "";
    string filename = "";
};
void html_escape(string &);
void console_format(vector<serverInfo> &);
void output_shell(string, string);
void output_command(string, string);

#endif