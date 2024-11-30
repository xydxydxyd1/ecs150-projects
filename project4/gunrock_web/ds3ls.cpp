#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <memory>
#include <exception>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}

void err() {
  cerr << "Directory not found" << endl;
  exit(1);
}

void print_entry(int inum, string name) {
  cout << inum << "\t" << name << endl;
}

void sort_and_print(vector<dir_ent_t>& dirs) {
  sort(dirs.begin(), dirs.end(), compareByName);
  for (dir_ent_t entry : dirs) {
    print_entry(entry.inum, entry.name);
  }
}

/// Resolve `path` to either a directory or a file. If a directory, returns a
/// vector that contains all directory entries in that directory. If a file,
/// returns a vector with length 1 that contains the directory entry
/// corresponding to the file.
///
/// If an error occurs, throws invalid_argument
vector<dir_ent_t> resolve_path(LocalFileSystem& fs, string path) {
  vector<dir_ent_t> out;
  if (path[0] != '/') {
    throw invalid_argument("path must be absolute (start with '/')");
  }

  int curr_i_num = 0; // corresponds to '/'
  string child_name;
  for (size_t i = 1; i < path.size(); i++) {
    if (path[i] == '/') {
      curr_i_num = fs.lookup(curr_i_num, child_name);
      if (curr_i_num < 0)
        throw invalid_argument("failed to lookup" + child_name);
      child_name.clear();
    }
    else
      child_name.push_back(path[i]);
  }
  // handle no trailing '/'
  bool has_trailling = child_name.empty();
  if (!has_trailling) {
    curr_i_num = fs.lookup(curr_i_num, child_name);
    if (curr_i_num < 0)
      throw invalid_argument("failed to lookup " + child_name);
  }

  inode_t inode;
  if (fs.stat(curr_i_num, &inode))
    throw invalid_argument("found file but is invalid");
  if (inode.type == UFS_DIRECTORY) {
    out.resize(inode.size / sizeof(dir_ent_t));
    if (fs.read(curr_i_num, out.data(), inode.size))
      throw invalid_argument("found file but can't read");
  }
  else if (inode.type == UFS_REGULAR_FILE) {
    if (has_trailling)
      throw invalid_argument("regular file cannot have trailing '/'");
    dir_ent_t entry;
    strcpy(entry.name, child_name.c_str());
    entry.inum = curr_i_num;
    out.push_back(entry);
  }
  return out;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }

  // parse command line arguments
  unique_ptr<Disk> disk(new Disk(argv[1], UFS_BLOCK_SIZE));
  unique_ptr<LocalFileSystem> fileSystem(new LocalFileSystem(disk.get()));
  string directory = string(argv[2]);

  int curr_inode_num = 0;
  inode_t curr_inode;
  if (fileSystem->stat(curr_inode_num, &curr_inode))
    err();
  vector<dir_ent_t> dir_ents;
  try {
      dir_ents = resolve_path(*fileSystem, directory);
  } catch (invalid_argument& e) {
    err();
  }
  sort_and_print(dir_ents);

  return 0;
}
