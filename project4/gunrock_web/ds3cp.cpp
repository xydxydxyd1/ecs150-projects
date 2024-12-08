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

  int fd = open(srcFile.c_str(), O_RDONLY);
  vector<char> content;
  char buf[1];
  int read_amt = read(fd, buf, 1);
  while (read_amt == 1) {
    content.push_back(*buf);
    read_amt = read(fd, buf, 1);
  }

  if (fileSystem.write(dstInode, content.data(), content.size()))
    err();
  
  return 0;
}
