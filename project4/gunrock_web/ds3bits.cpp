#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <memory>
#include <iomanip>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  // Parse command line arguments
  unique_ptr<Disk> disk(new Disk(argv[1], UFS_BLOCK_SIZE));
  unique_ptr<LocalFileSystem> file_system(new LocalFileSystem(disk.get()));
  unique_ptr<super_t> super(new super_t);
  file_system->readSuperBlock(super.get());

  unique_ptr<unsigned char[]> inode_bitmap(
      new unsigned char[super->inode_bitmap_len * UFS_BLOCK_SIZE]);
  unique_ptr<unsigned char[]> data_bitmap(
      new unsigned char[super->data_bitmap_len * UFS_BLOCK_SIZE]);
  file_system->readInodeBitmap(super.get(), inode_bitmap.get());
  file_system->readDataBitmap(super.get(), data_bitmap.get());

  cout << "Super" << endl;
  cout << "inode_region_addr " << super->inode_region_addr << endl;
  cout << "inode_region_len " << super->inode_region_len << endl;
  cout << "num_inodes " << super->num_inodes << endl;
  cout << "data_region_addr " << super->data_region_addr << endl;
  cout << "data_region_len " << super->data_region_len << endl;
  cout << "num_data " << super->num_data << endl;

  cout << endl;
  cout << "Inode bitmap" << endl;
  for (int i = 0; i < super->num_inodes / 8; i++) {
    cout  << (int)inode_bitmap[i] << " ";
  }
  cout << endl;
  cout << endl;

  cout << "Data bitmap" << endl;
  for (int i = 0; i < super->num_data / 8; i++) {
    cout  << (int)data_bitmap[i] << " ";
  }
  cout << endl;
  /*
  inode_region_addr 3
  inode_region_len 1
  num_inodes 32
  data_region_addr 4
  data_region_len 32
  num_data 32

  Inode bitmap
  15 0 0 0 

  Data bitmap
  15 0 0 0 
  */


  
  return 0;
}
