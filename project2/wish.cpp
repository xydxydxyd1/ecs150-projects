// `wish`, the Wisconsin Shell
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
using namespace std;

const string PROMPT = "wish> ";
const string ERR_MSG = "An error has occurred";

/// Main program in interactive mode
void interactive() {
    bool is_running = true;
    string input;
    while (is_running) {
        cout << PROMPT;
        getline(cin, input);
    }
}

/// Main program in batch mode
void batch(char** filenames, int num_files) {
}


// Commands

typedef function<void(string, const ostream&)> command;

void builtin_exit(string, const ostream&);
void builtin_cd(string, const ostream&);
void builtin_path(string, const ostream&);

unordered_map<string, command> builtin_cmds{
    {"exit", builtin_exit},
    {"cd", builtin_cd},
    {"path", builtin_path},
};

/// Run `cmds`
void run(const vector<string> cmds, const ostream& output) {
    for (string cmd : cmds) {
        try {
            command builtin_cmd = builtin_cmds.at(cmd);
            continue;
        } catch(out_of_range err) {}
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        interactive();
    }
    else {
        batch(&(argv[1]), argc - 1);
    }
    return 0;
}
