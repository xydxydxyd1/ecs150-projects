#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

void err() {
  cerr << "Error removing entry" << endl;
  exit(1);
}


int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile parentInode entryName" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem fileSystem(&disk);
  int parentInode = stoi(argv[2]);
  string entryName = string(argv[3]);

  if (fileSystem.unlink(parentInode, entryName))
    err();

  return 0;
}
