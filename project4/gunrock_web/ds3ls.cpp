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

/*
  Use this function with std::sort for directory entries
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}
*/

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }

  // parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  /*
  string directory = string(argv[2]);
  */

  unique_ptr<inode_t> root_dir(new inode_t);
  int ret = fileSystem->stat(0, root_dir.get());
  cout << "Ret: " << ret << endl;
  cout << "Type: " << root_dir->type << endl;
  cout << "Size: " << root_dir->size << endl;

  delete disk;
  delete fileSystem;
  return 0;
}
