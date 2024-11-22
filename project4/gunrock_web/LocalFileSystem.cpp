#include "LocalFileSystem.h"

#include <assert.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "ufs.h"

using namespace std;

/// Read `len` bytes from `disk` into `dest`, starting from byte address
/// `addr`
///
/// Errors are thrown at the Disk level
void read_bytes(Disk *disk, int addr, int len, char* dest) {
  char buf[UFS_BLOCK_SIZE];
  int write_offset = 0;
  int unread_len = len;
  int blk_num = addr / UFS_BLOCK_SIZE;
  const int offset = addr % UFS_BLOCK_SIZE;

  // Initial read has offset
  int read_amt = UFS_BLOCK_SIZE - offset;
  read_amt = unread_len > read_amt ? read_amt : unread_len;
  disk->readBlock(blk_num, buf);
  memcpy(dest, buf + offset, read_amt);

  blk_num++;
  unread_len -= read_amt;
  write_offset += read_amt;
  while (unread_len > 0) {
    read_amt = unread_len > UFS_BLOCK_SIZE ? UFS_BLOCK_SIZE : unread_len;
    disk->readBlock(blk_num, buf);
    memcpy(dest + write_offset, buf, read_amt);
    blk_num++;
    unread_len -= read_amt;
    write_offset += read_amt;
  }
}

/// Check to see if the bit `index` corresponds to on `bitmap` is set. Returns
/// true if set, false if otherwise. Assume `index` is valid in `bitmap`
bool check_bitmap(const unsigned char* bitmap, int index) {
  const int offset = index % sizeof(unsigned char);
  const int bitmap_index = index / sizeof(unsigned char);
  return (bitmap[bitmap_index] & (1 << offset)) != 0;
}

LocalFileSystem::LocalFileSystem(Disk *disk) { this->disk = disk; }

void LocalFileSystem::readSuperBlock(super_t *super) {
  read_bytes(disk, 0, sizeof(super_t), (char*)super);
}

void LocalFileSystem::readInodeBitmap(super_t *super,
                                      unsigned char *inodeBitmap) {
  read_bytes(disk,
      super->inode_bitmap_addr * UFS_BLOCK_SIZE,
      super->inode_bitmap_len * UFS_BLOCK_SIZE,
      (char*)inodeBitmap);
}

void LocalFileSystem::writeInodeBitmap(super_t *super,
                                       unsigned char *inodeBitmap) {}

void LocalFileSystem::readDataBitmap(super_t *super,
                                     unsigned char *dataBitmap) {
  read_bytes(disk,
      super->data_bitmap_addr * UFS_BLOCK_SIZE,
      super->data_bitmap_len * UFS_BLOCK_SIZE,
      (char*)dataBitmap);
}

void LocalFileSystem::writeDataBitmap(super_t *super,
                                      unsigned char *dataBitmap) {}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  read_bytes(disk,
      super->inode_region_addr * UFS_BLOCK_SIZE,
      super->inode_region_len * UFS_BLOCK_SIZE,
      (char*)inodes);
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  return 0;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  unique_ptr<super_t> super(new super_t);
  readSuperBlock(super.get());
  unique_ptr<unsigned char[]> inode_bitmap(
      new unsigned char[super->inode_region_len * UFS_BLOCK_SIZE]);
  readInodeBitmap(super.get(), inode_bitmap.get());

  if (inodeNumber >= super->num_inodes
      || !check_bitmap(inode_bitmap.get(), inodeNumber)) {
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
  int write_offset = 0;
  int unread_len = size;
  for (auto direct_ptr : inode->direct) {
    if (unread_len == 0)
      break;
    disk->readBlock(direct_ptr, read_blk);
    int read_amt = unread_len > UFS_BLOCK_SIZE ? UFS_BLOCK_SIZE : unread_len;
    memcpy((char*)buffer + write_offset, read_blk, read_amt);

    unread_len -= read_amt;
    write_offset += read_amt;
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
