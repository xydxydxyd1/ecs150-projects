# Specifications

## I/O
* Interactive mode repeatedly prompts user for input
* Batch mode takes in files with each line as input
* All input ends upon `exit` or character `EOF`.

## Execution

Execute some commands

* `add(const vector<string> cmds)`

## Preprocessing

Preprocess what to execute

Interface:
* `add(const vector<string> cmds)` - Add `cmds` to be processed, where each element is
  a single command. This is called by *Input*.
