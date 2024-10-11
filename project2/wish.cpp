// `wish`, the Wisconsin Shell
#include <functional>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#include <wait.h>
using namespace std;

const string PROMPT = "wish> ";
const string ERR_MSG = "An error has occurred";

void run(const string cmd, const ostream& output);

/// Main program in interactive mode
void interactive() {
    bool is_running = true;
    string input;
    while (is_running) {
        cout << PROMPT;
        getline(cin, input);
        run(input, cout);
    }
}

/// Main program in batch mode
void batch(char** filenames, int num_files) {
}


// Commands

vector<string> paths{"/bin"};

typedef function<void(string, const ostream&)> command;

void builtin_exit(string, const ostream&) {}
void builtin_cd(string, const ostream&) {}
void builtin_path(string, const ostream&) {}

unordered_map<string, command> builtin_cmds{
    {"exit", builtin_exit},
    {"cd", builtin_cd},
    {"path", builtin_path},
};

/// Return the first word in `cmd`
string get_firstword(const string& cmd) {
    istringstream cmd_stream(cmd);
    string id;
    cmd_stream >> id;
    return id;
}

/// Return the full path of an executable for `name`. If one cannot be found,
/// return an empty string.
///
/// Args:
/// name - the name of the executable without arguments
string get_executable(const string& name) {
    string out;
    for (string path : paths) {
        string executable = path.append("/");
        executable.append(name);
        if (access(executable.c_str(), X_OK) == 0) {
            out = executable;
            break;
        }
    }
    return out;
}

/// Run a `cmd`
void run(const string cmd, const ostream& output) {
    istringstream cmd_stream(cmd);
    string name;
    cmd_stream >> name;

    // Empty line
    if (name.size() == 0)
        return;

    try {
        // Built-in command
        command builtin_cmd = builtin_cmds.at(name);
        builtin_cmd(cmd, output);
        return;
    } catch(out_of_range err) {}

    // Everything else

    string executable = get_executable(name);
    if (executable.size() == 0) {
        cerr << ERR_MSG << endl;
        return;
    }

    vector<string> args;
    args.push_back(name);
    while (!cmd_stream.eof()) {
        string arg;
        cmd_stream >> arg;
        args.push_back(arg);
    }
    char** fmt_args = new char*[args.size() + 1];
    for (int i = 0; i < args.size(); i++) {
        fmt_args[i] = new char[args[i].size()];
        strcpy(fmt_args[i], args[i].c_str());
    }
    fmt_args[args.size()] = nullptr;

    if (int pid = fork()) {
        wait(nullptr);
    }
    else {
        execv(executable.c_str(), fmt_args);
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
