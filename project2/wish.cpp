// `wish`, the Wisconsin Shell
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <unordered_map>
#include <vector>
#include <wait.h>
using namespace std;

// TODO: Error check on syscalls
// IN PROGRESS: Concurrency

const string PROMPT = "wish> ";
const string ERR_MSG = "An error has occurred";

void interactive();
void batch(char** filenames, int num_files);

void run_expr(const string expr);
string get_firstword(const string& cmd);
string get_executable(const string& name);

void builtin_exit(vector<string>);
void builtin_cd(vector<string>);
void builtin_path(vector<string>);

/// Main program in interactive mode
void interactive() {
    bool is_running = true;
    string input;
    while (is_running) {
        cout << PROMPT;
        try {
            getline(cin, input);
            run_expr(input);
        } catch (invalid_argument& e) {
            cerr << ERR_MSG << endl;
            return;
        }
    }
}

/// Main program in batch mode
void batch(char** filenames, int num_files) {
}


// Commands

vector<string> paths{"/bin"};

/// Type for a builtin command. Takes in a vector of arguments for the command,
/// with the first argument being the name of the command (by convention)
///
/// If command failed to execute, throws `std::invalid_argument` with the
/// correct usage.

typedef function<void(vector<string>)> BuiltinCmd;

void builtin_exit(vector<string> args) {
    exit(0);
}
void builtin_cd(vector<string> args) {
    if (args.size() != 2)
        throw invalid_argument("cd directory");
    chdir(args[1].c_str());
}
void builtin_path(vector<string> args) {
    paths = vector<string>(args.begin() + 1, args.end());
}

unordered_map<string, BuiltinCmd> builtin_cmds{
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

/// Extract a token from `stream`. Throw `invalid_argument` if no token can be
/// extracted.
///
/// A token is the smallest lexical unit of a command. It delimited by spaces
/// except the following cases:
/// - '>': redirection token, which is a token that delimites
string get_token(istream& stream) {
    string token;
    char quotechar = 0;
    while (!stream.eof()) {
        char buf = stream.peek();
        if (buf == '>') {
            if (token.empty()) {
                stream.get();
                return ">";
            }
            else
                break;
        }

        stream.get();
        if (buf == EOF) {
            break;
        }
        if (isspace(buf)) {
            if (token.empty())
                continue;
            else if (quotechar == 0)
                break;
        }
        if (buf == '"' || buf == '\'') {
            if (quotechar == 0) {
                quotechar = buf;
                continue;
            }
            else if (quotechar == buf) {
                quotechar = 0;
                continue;
            }
        }

        token.push_back(buf);
    }
    if (token.empty())
        throw invalid_argument("failed to extract token");
    return token;
}

/// Contains information of a command and methods to construct/execute it
class Command {
  private:
    string out_filename;
  public:
    string cmd_name;
    vector<string> args;

    /// Read a command from the stream.
    ///
    /// A command is delimited by either newline '\n' or concurrency '&'. In
    /// each case, the delimiter is consumed.
    Command(istream& stream) {
        cmd_name = get_token(stream);
        if (cmd_name.size() == 0)
            return;
        args.push_back(cmd_name);

        while (!stream.eof()) {
            string token = get_token(stream);
            if (token == ">") {
                token = get_token(stream);
                try {
                    token = get_token(stream);
                    // No more tokens should be available, which would throw
                    // invalid_argument before here.
                    cerr << ERR_MSG << endl;
                    return;
                } catch (invalid_argument& e) {}
                out_filename = token.c_str();
                break;
            }
            args.push_back(token);
        }
    }

    /// Execute the command
    void execute() {
        int fd_out;
        if (out_filename.empty())
            fd_out = STDOUT_FILENO;
        else {
            fd_out = open(out_filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
            if (fd_out  == -1)
                throw invalid_argument("failed to open file " + out_filename);
        }

        // Built-in command
        try {
            BuiltinCmd builtin_cmd = builtin_cmds.at(cmd_name);
            builtin_cmd(args);
            return;
        } catch(const out_of_range& err) {}

        // Everything else

        string executable = get_executable(cmd_name);
        if (executable.size() == 0) {
            cerr << ERR_MSG << endl;
            return;
        }

        char** fmt_args = new char*[args.size() + 1];
        for (int i = 0; i < args.size(); i++) {
            fmt_args[i] = new char[args[i].size()];
            strcpy(fmt_args[i], args[i].c_str());
        }
        fmt_args[args.size()] = nullptr;

        if (int pid = fork()) {
            wait(nullptr);
            return;
        }
        else {
            if (fd_out != STDOUT_FILENO) {
                if (dup2(fd_out, STDOUT_FILENO) == -1) {
                    cerr << ERR_MSG << endl;
                    return;
                }
            }
            execv(executable.c_str(), fmt_args);
            cerr << ERR_MSG << endl;
            return;
        }
    }
};

/// Run an expression, which includes commands combined with redirection and
/// flow control. Throw `invalid_argument` if command failed to be
/// executed
void run_expr(const string expr) {
    // Preprocessing
    istringstream cmd_stream(expr);

    Command cmd(cmd_stream);
    cmd.execute();
}

int main(int argc, char** argv) {
    if (argc == 1) {
        interactive();
    }
    else if (argc == 2) {
        batch(&(argv[1]), argc - 1);
    }
    else {
        cerr << ERR_MSG << endl;
        return 1;
    }
    return 0;
}
