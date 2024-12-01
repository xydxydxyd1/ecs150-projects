#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

void err() {
  cerr << "Error reading file" << endl;
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem fileSystem(&disk);
  int inodeNumber = stoi(argv[2]);

  inode_t inode;
  if (fileSystem.stat(inodeNumber, &inode) || inode.type == UFS_DIRECTORY) {
    err();
  }

  cout << "File blocks" << endl;
  int num_blocks = inode.size / UFS_BLOCK_SIZE + (inode.size % UFS_BLOCK_SIZE > 0);
  for (int i = 0; i < num_blocks; i++) {
    cout << (int)inode.direct[i] << endl;
  }
  cout << endl;

  cout << "File data" << endl;
  char* buf = new char[inode.size];
  if (fileSystem.read(inodeNumber, buf, inode.size))
    err();
  for (int i = 0; i < inode.size; i++) {
    cout << buf[i];
  }
  cout.flush();

  delete[] buf;
  return 0;
}
