// `wish`, the Wisconsin Shell
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <wait.h>
using namespace std;

// TODO: Error check on syscalls
// Batch

const string PROMPT = "wish> ";
const string ERR_MSG = "An error has occurred";

void wish();

void run_expr(const string expr);
string get_firstword(const string& cmd);
string get_executable(const string& name);

void builtin_exit(vector<string>);
void builtin_cd(vector<string>);
void builtin_path(vector<string>);

/// Main program in interactive mode
void interactive() {
    string input;
    while (!cin.eof()) {
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
void batch(istream& in) {
    string input;
    while (!in.eof()) {
        try {
            getline(in, input);
            run_expr(input);
        } catch (invalid_argument& e) {
            cerr << ERR_MSG << endl;
            return;
        }
    }
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
        throw invalid_argument("cd [directory]");
    chdir(args[1].c_str());
}
void builtin_path(vector<string> args) {
    paths = vector<string>(args.begin() + 1, args.end());
}

const unordered_map<string, BuiltinCmd> builtin_cmds{
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
        // Non-space delimiters
        if (buf == '>' || buf == '&') {
            if (token.empty()) {
                stream.get();
                token.push_back(buf);
            }
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
    string cmd_name;
    vector<string> args;

    bool parsed = false;
    int fd_out;
    /// nullptr if not builtin_cmd
    const BuiltinCmd* builtin_cmd = nullptr;
    /// empty string if builtin_cmd
    string executable;
    char ** fmt_args = nullptr;
  public:
    /// Read a command from the stream.
    ///
    /// A command is delimited by either newline '\n' or concurrency '&'. In
    /// each case, the delimiter is consumed.
    Command(istream& stream) {
        try {
            cmd_name = get_token(stream);
        } catch (invalid_argument& e) {
            return;
        }
        args.push_back(cmd_name);

        while (!stream.eof()) {
            string token;
            try {
                token = get_token(stream);
            } catch (invalid_argument& err) {
                break;
            }
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
            if (token == "&") {
                return;
            }
            args.push_back(token);
        }
    }

    ~Command() {
        if (fmt_args == nullptr)
            return;

        for (int i = 0; fmt_args[i] != nullptr; i++) {
            delete fmt_args[i];
        }
        delete fmt_args;
    }

    /// Parse the command before executing, performing error checks
    /// and provide information about the command. If error, throws
    /// `invalid_argument`
    void parse() {
        if (cmd_name.empty()) {
            parsed = true;
            return;
        }

        if (out_filename.empty())
            fd_out = STDOUT_FILENO;
        else {
            fd_out = open(out_filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
            if (fd_out  == -1)
                throw invalid_argument("failed to open file " + out_filename);
        }

        try {
            builtin_cmd = &(builtin_cmds.at(cmd_name));
            parsed = true;
            return;
        } catch(const out_of_range& err) {}

        executable = get_executable(cmd_name);
        if (executable.size() == 0) {
            throw invalid_argument("command " + cmd_name + " not found");
        }

        fmt_args = new char*[args.size() + 1];
        for (int i = 0; i < args.size(); i++) {
            fmt_args[i] = new char[args[i].size()];
            strcpy(fmt_args[i], args[i].c_str());
        }
        fmt_args[args.size()] = nullptr;

        parsed = true;
    }

    bool is_builtin() {
        if (!parsed)
            throw domain_error("command not parsed yet");
        return builtin_cmd != nullptr;
    }

    bool is_empty() {
        return cmd_name.empty();
    }

    /// Execute the command.
    ///
    /// If the command is not parsed, throws `domain_error`. If the command is
    /// builtin, run in the same process and return. Otherwise, replace current
    /// process with executing program.
    void execute() {
        if (!parsed)
            throw domain_error("command is not parsed");

        if (cmd_name.empty())
            return;

        if (builtin_cmd != nullptr) {
            (*builtin_cmd)(args);
            return;
        }

        if (dup2(fd_out, STDOUT_FILENO) == -1) {
            throw invalid_argument("redirection error");
        }
        execv(executable.c_str(), fmt_args);
        throw invalid_argument("non-builtin execution failed");
    }

    string to_string() {
        ostringstream out;
        out << cmd_name;
        for (string arg : args) {
            out << " " << arg;
        }
        return out.str();
    }
};

/// Run an expression
///
/// An expression is a single line of instruction, including zero or more
/// commands combined with redirection and flow control. Throw
/// `invalid_argument` if expression failed to be executed
void run_expr(const string expr) {
    // Preprocessing
    istringstream cmd_stream(expr);

    vector<Command> cmds;
    vector<pid_t> pids;
    while (!cmd_stream.eof()) {
        Command cmd(cmd_stream);
        if (!cmd.is_empty())
            cmds.push_back(std::move(cmd));
    }

    for (Command& cmd : cmds) {
        cmd.parse();
        if (cmd.is_builtin()) {
            if (cmds.size() > 1)
                throw invalid_argument("builtin command in parallel expression");
            cmd.execute();
        }
        else if (!cmd.is_empty()) {
            int pid = fork();
            if (pid == 0) {
                cmd.execute();
                throw invalid_argument("failed to execute command \"" + cmd.to_string() + "\"");
            }
            pids.push_back(pid);
        }
    }

    for (pid_t pid : pids) {
        waitpid(pid, nullptr, 0);
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        interactive();
    }
    else if (argc == 2) {
        ifstream file(argv[1]);
        batch(file);
    }
    else {
        cerr << ERR_MSG << endl;
        return 1;
    }
    return 0;
}
