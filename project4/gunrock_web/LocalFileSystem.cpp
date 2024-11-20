#include "LocalFileSystem.h"

#include <assert.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "ufs.h"

using namespace std;

LocalFileSystem::LocalFileSystem(Disk *disk) { this->disk = disk; }

void LocalFileSystem::readSuperBlock(super_t *super) {
  char buf[UFS_BLOCK_SIZE];
  disk->readBlock(0, buf);
  memcpy(super, buf, sizeof(super_t));
}

void LocalFileSystem::readInodeBitmap(super_t *super,
                                      unsigned char *inodeBitmap) {}

void LocalFileSystem::writeInodeBitmap(super_t *super,
                                       unsigned char *inodeBitmap) {}

void LocalFileSystem::readDataBitmap(super_t *super,
                                     unsigned char *dataBitmap) {}

void LocalFileSystem::writeDataBitmap(super_t *super,
                                      unsigned char *dataBitmap) {}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  char blk_buf[UFS_BLOCK_SIZE];
  int write_i = 0;
  for (int read_blk_i = 0; read_blk_i < super->inode_region_len; read_blk_i++) {
    disk->readBlock(super->inode_region_addr + read_blk_i, blk_buf);
    int read_i = 0;
    while (write_i < super->num_inodes &&
           read_i * sizeof(char) + sizeof(inode_t) <
               UFS_BLOCK_SIZE * sizeof(char)) {
      memcpy(&inodes[write_i], &blk_buf[read_i], sizeof(inode_t));
      write_i++;
      read_i++;
    }
  }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {}

int LocalFileSystem::lookup(int parentInodeNumber, string name) { return 0; }

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  unique_ptr<super_t> super(new super_t);
  readSuperBlock(super.get());

  /* TODO: Check bitmap <19-11-24, Eric Xu> */
  if (inodeNumber >= super->num_inodes) {
    return EINVALIDINODE;
  }

  unique_ptr<inode_t[]> inode_region(new inode_t[super->num_inodes]);
  readInodeRegion(super.get(), inode_region.get());
  memcpy(inode, &inode_region[inodeNumber], sizeof(inode_t));
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  unique_ptr<inode_t> inode(new inode_t);
  int ret = stat(inodeNumber, inode.get());
  if (ret != 0)
    return EINVALIDINODE;
  if (inode->size < size)
    return EINVALIDSIZE;

  char read_blk[UFS_BLOCK_SIZE];
  for (auto direct_ptr : inode->direct) {
    if (size == 0)
      break;
    disk->readBlock(direct_ptr, read_blk);
    int read_amt = size > UFS_BLOCK_SIZE ? UFS_BLOCK_SIZE : size;
    size -= read_amt;
    memcpy(buffer, read_blk, read_amt);
  }
  return 0;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) { return 0; }
