#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Disk.h"
#include "LocalFileSystem.h"
#include "StringUtils.h"
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

/// Resolve `path` to a directory entry
///
/// If an error occurs, throws invalid_argument
dir_ent_t resolve_path(LocalFileSystem& fs, string path) {
  if (path[0] != '/') {
    throw invalid_argument("path must be absolute (start with '/')");
  }

  int curr_i_num = 0;  // corresponds to '/'
  string child_name;
  for (size_t i = 1; i < path.size(); i++) {
    if (path[i] == '/') {
      curr_i_num = fs.lookup(curr_i_num, child_name);
      if (curr_i_num < 0)
        throw invalid_argument("failed to lookup " + child_name);
      child_name.clear();
    } else
      child_name.push_back(path[i]);
  }
  bool has_trailing_delim = child_name.empty();
  if (!has_trailing_delim) {
    curr_i_num = fs.lookup(curr_i_num, child_name);
    if (curr_i_num < 0)
      throw invalid_argument("failed to lookup " + child_name);
  }

  inode_t inode;
  if (fs.stat(curr_i_num, &inode))
    throw invalid_argument("found file but is invalid");
  if (inode.type == UFS_REGULAR_FILE && has_trailing_delim)
    throw invalid_argument("regular file cannot have trailing '/'");
  dir_ent_t out;
  out.inum = curr_i_num;
  strcpy(out.name, child_name.c_str());
  return out;
}

/// Read a directory entry. If it is a directory, return all files inside the
/// directory. If it is a regular file, return a vector of size 1 containing
/// only `dir_ent`
///
/// Throws `invalid_argument` if error occurs
vector<dir_ent_t> read_entry(LocalFileSystem& fs, dir_ent_t dir_ent) {
  inode_t inode;
  if (fs.stat(dir_ent.inum, &inode))
    throw invalid_argument("file is invalid");
  vector<dir_ent_t> out;
  if (inode.type == UFS_DIRECTORY) {
    out.resize(inode.size / sizeof(dir_ent_t));
    if (fs.read(dir_ent.inum, out.data(), inode.size))
      throw invalid_argument("found file but can't read");
  } else if (inode.type == UFS_REGULAR_FILE) {
    out.push_back(dir_ent);
  }
  return out;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }

  // parse command line arguments
  Disk disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem fileSystem(&disk);
  string directory = string(argv[2]);

  int curr_inode_num = 0;
  inode_t curr_inode;
  if (fileSystem.stat(curr_inode_num, &curr_inode)) err();
  vector<dir_ent_t> dir_ents;
  try {
    dir_ent_t entry = resolve_path(fileSystem, directory);
    dir_ents = read_entry(fileSystem, entry);
  } catch (invalid_argument& e) {
    err();
  }
  sort_and_print(dir_ents);

  return 0;
}
