// `wish`, the Wisconsin Shell
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
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

void builtin_ls(string) {
}

typedef function<void(string)> command;

unordered_map<string, command> builtin_cmds{
    {"ls", builtin_ls},
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
