#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "html_handler.hpp"

using namespace std;

void html_escape(string &content) {
    vector<char> esc_from{'&', '<', '>', '\n', '\'', '\"'};
    vector<string> esc_to{"&amp", "&lt;", "&gt;", "&NewLine;", "&apos;", "&quot;"};

    for (int i = 0; i < esc_from.size(); i++) {
        int cur_i = -1, esc_i = content.find(esc_from[i]);
        while (esc_i != string::npos && esc_i > cur_i) {
            cur_i = esc_i;
            content.replace(cur_i, 1, esc_to[i]);
            esc_i = content.find(esc_from[i]);
        }
    }
}

void console_format(vector<serverInfo> &sinfo) {
    cout << "Content-type: text/html\r\n\r\n";

    cout << "<!DOCTYPE html>\r\n";
    cout << "<html lang=\"en\">\r\n";
    cout << "  <head>\r\n";
    cout << "    <meta charset=\"UTF-8\" />\r\n";
    cout << "    <title>NP Project 3 Console</title>\r\n";
    cout << "    <link\r\n";
    cout << "      rel=\"stylesheet\"\r\n";
    cout << "      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\r\n";
    cout << "      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\r\n";
    cout << "      crossorigin=\"anonymous\"\r\n";
    cout << "    />\r\n";
    cout << "    <link\r\n";
    cout << "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\r\n";
    cout << "      rel=\"stylesheet\"\r\n";
    cout << "    />\r\n";
    cout << "    <link\r\n";
    cout << "      rel=\"icon\"\r\n";
    cout << "      type=\"image/png\"\r\n";
    cout << "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\r\n";
    cout << "    />\r\n";
    cout << "    <style>\r\n";
    cout << "      * {\r\n";
    cout << "        font-family: 'Source Code Pro', monospace;\r\n";
    cout << "        font-size: 1rem !important;\r\n";
    cout << "      }\r\n";
    cout << "      body {\r\n";
    cout << "        background-color: #212529;\r\n";
    cout << "      }\r\n";
    cout << "      pre {\r\n";
    cout << "        color: #cccccc;\r\n";
    cout << "      }\r\n";
    cout << "      b {\r\n";
    cout << "        color: #01b468;\r\n";
    cout << "      }\r\n";
    cout << "    </style>\r\n";
    cout << "  </head>\r\n";
    cout << "  <body>\r\n";
    cout << "    <table class=\"table table-dark table-bordered\">\r\n";
    cout << "      <thead>\r\n";
    cout << "        <tr>\r\n";
    for (int h = 0; h < sinfo.size(); h++) {
        if (sinfo[h].host == "") {
            continue;
        }
        cout << "          <th scope=\"col\">" + sinfo[h].host + ":" + sinfo[h].port + "</th>\r\n";
        // cout << "          <th scope=\"col\">" + string("thisisatest") + ":" + sinfo[h].port + "</th>\r\n";
    }
    cout << "        </tr>\r\n";
    cout << "      </thead>\r\n";
    cout << "      <tbody>\r\n";
    cout << "        <tr>\r\n";
    for (int h = 0; h < sinfo.size(); h++) {
        if (sinfo[h].host == "") {
            continue;
        }
        string id = "s" + to_string(h);
        cout << "          <td><pre id=\"" + id + "\" class=\"mb-0\"></pre></td>\r\n";
    }
    cout << "        </tr>\r\n";
    cout << "      </tbody>\r\n";
    cout << "    </table>\r\n";
    cout << "  </body>\r\n";
    cout << "</html>\r\n";
}

void endline_parser(string &s) {
    int r = s.find("\r");
    while (r != string::npos) {
        s.erase(r, 1);
        r = s.find("\r");
    }
    // int n = _cmd.find("\n");
    // while (n != string::npos) {
    //     _cmd.erase(n, 1);
    //     n = _cmd.find("\n");
    // }
}

void output_shell(string session, string content) {
    endline_parser(content);
    html_escape(content);
    string shell =  "<script>document.getElementById('" + session + "').innerHTML +='" + content + "';</script>";
    // cerr << shell << '\n' << flush;
    cout << shell << flush;
}

void output_command(string session, string content) {
    endline_parser(content);
    html_escape(content);
    string shell =  "<script>document.getElementById('" + session + "').innerHTML +='<b>" + content + "</b>';</script>";
    // cerr << shell << '\n' << flush;
    cout << shell << flush;
}