// `wish`, the Wisconsin Shell
#include <iostream>
#include <string>
using namespace std;

const string PROMPT = "wish> ";
const string ERR_MSG = "An error has occurred";

/// Main program in interactive mode
void interactive() {
    bool is_running = true;
    string input;
    while (is_running) {
        cout << PROMPT;
        cin >> input;
        cout << input << endl;
    }
}

/// Main program in batch mode
void batch(char** filenames, int num_files) {
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
