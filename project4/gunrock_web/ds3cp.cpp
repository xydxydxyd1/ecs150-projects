#include <fstream>
#include <vector>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

void err() {
  cerr << "Could not write to dst_file" << endl;
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile src_file dst_inode" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img dthread.cpp 3" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem fileSystem(&disk);
  string srcFile = string(argv[2]);
  int dstInode = stoi(argv[3]);

  ifstream file(srcFile);
  vector<char> buf;
  while (file.good()) {
    buf.push_back(file.get());
  }
  if (buf[buf.size() - 1] == EOF)
    buf.pop_back();

  if (fileSystem.write(dstInode, buf.data(), buf.size()))
    err();
  
  return 0;
}
