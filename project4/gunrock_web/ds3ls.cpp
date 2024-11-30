#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <memory>

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
  /*
  string directory = string(argv[2]);
  */

  int curr_inode_num = 0;
  inode_t curr_inode;
  if (fileSystem->stat(curr_inode_num, &curr_inode))
    err();
  vector<dir_ent_t> curr_dir;
  curr_dir.resize(curr_inode.size / sizeof(dir_ent_t));
  if (fileSystem->read(curr_inode_num, curr_dir.data(), curr_inode.size))
    err();
  sort_and_print(curr_dir);

  return 0;
}
