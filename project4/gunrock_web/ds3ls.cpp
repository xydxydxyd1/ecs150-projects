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
  unique_ptr<Disk> disk(new Disk(argv[1], UFS_BLOCK_SIZE));
  unique_ptr<LocalFileSystem> fileSystem(new LocalFileSystem(disk.get()));
  /*
  string directory = string(argv[2]);
  */

  dir_ent_t root_dir[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
  int ret = fileSystem->read(0, root_dir, 2*32);
  cout << "Ret: " << ret << endl;
  cout << "Type: " << root_dir[1].inum << endl;
  cout << "Name: " << root_dir[1].name << endl;

  return 0;
}
