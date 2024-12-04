#include "LocalFileSystem.h"

#include <assert.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "ufs.h"

using namespace std;

/// Convert number of bytes to number of blocks needed to store the bytes
size_t bytes_to_blks(size_t num_bytes) {
  return num_bytes / UFS_BLOCK_SIZE + (num_bytes % UFS_BLOCK_SIZE > 0);
}

/// Read `len` bytes from `disk` into `dest`, starting from byte address
/// `addr`
///
/// Errors are thrown at the Disk level
void read_bytes(Disk *disk, int addr, int len, void* dest) {
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
    memcpy((char*)dest + write_offset, buf, read_amt);
    blk_num++;
    unread_len -= read_amt;
    write_offset += read_amt;
  }
}

/// Write `len` bytes from `src` into `disk`, starting from byte address
/// `addr`.
///
/// Does not start or stop transaction. Errors are thrown at the Disk level
void write_bytes(Disk *disk, int addr, int len, const void* src) {
  char buf[UFS_BLOCK_SIZE];
  int write_offset = 0;
  int unwritten_len = len;
  int blk_num = addr / UFS_BLOCK_SIZE;
  const int offset = addr % UFS_BLOCK_SIZE;

  // Initial write has offset
  int write_amt = UFS_BLOCK_SIZE - offset;
  write_amt = unwritten_len > write_amt ? write_amt : unwritten_len;
  disk->readBlock(blk_num, buf);
  memcpy(buf + offset, src, write_amt);
  disk->writeBlock(blk_num, buf);

  blk_num++;
  unwritten_len -= write_amt;
  write_offset += write_amt;
  while (unwritten_len > 0) {
    write_amt = unwritten_len > UFS_BLOCK_SIZE ? UFS_BLOCK_SIZE : unwritten_len;
    disk->readBlock(blk_num, buf);
    memcpy(buf, (char*)src + write_offset, write_amt);
    blk_num++;
    unwritten_len -= write_amt;
    write_offset += write_amt;
  }
}

/// Check to see if the bit `index` corresponds to on `bitmap` is set. Returns
/// true if set, false if otherwise. Assume `index` is valid in `bitmap`
bool is_allocated(const unsigned char* bitmap, int index) {
  const int size_in_bits = sizeof(unsigned char) * 8;
  const int offset = index % size_in_bits;
  const int bitmap_index = index / size_in_bits;
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
                                       unsigned char *inodeBitmap) {
  write_bytes(disk,
      super->inode_bitmap_addr * UFS_BLOCK_SIZE,
      super->inode_bitmap_len * UFS_BLOCK_SIZE,
      (char*)inodeBitmap);
}

void LocalFileSystem::readDataBitmap(super_t *super,
                                     unsigned char *dataBitmap) {
  read_bytes(disk,
      super->data_bitmap_addr * UFS_BLOCK_SIZE,
      super->data_bitmap_len * UFS_BLOCK_SIZE,
      (char*)dataBitmap);
}

void LocalFileSystem::writeDataBitmap(super_t *super,
                                      unsigned char *dataBitmap) {
  write_bytes(disk,
      super->data_bitmap_addr * UFS_BLOCK_SIZE,
      super->data_bitmap_len * UFS_BLOCK_SIZE,
      (char*)dataBitmap);
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  read_bytes(disk,
      super->inode_region_addr * UFS_BLOCK_SIZE,
      super->inode_region_len * UFS_BLOCK_SIZE,
      (char*)inodes);
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
  write_bytes(disk,
      super->inode_region_addr * UFS_BLOCK_SIZE,
      super->inode_region_len * UFS_BLOCK_SIZE,
      (char*)inodes);
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  inode_t inode;
  if (stat(parentInodeNumber, &inode))
    return -EINVALIDINODE;
  if (inode.type != UFS_DIRECTORY)
    return -EINVALIDINODE;
  vector<dir_ent_t> dir;
  dir.resize(inode.size / sizeof(dir_ent_t));
  if (read(parentInodeNumber, dir.data(), inode.size))
    return -EINVALIDINODE;

  for (dir_ent_t entry : dir) {
    if (entry.name == name)
      return entry.inum;
  }
  return -ENOTFOUND;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  super_t super;
  readSuperBlock(&super);
  unique_ptr<unsigned char[]> inode_bitmap(
      new unsigned char[super.inode_bitmap_len * UFS_BLOCK_SIZE]);
  readInodeBitmap(&super, inode_bitmap.get());

  if (inodeNumber >= super.num_inodes
      || !is_allocated(inode_bitmap.get(), inodeNumber)) {
    return -EINVALIDINODE;
  }

  unique_ptr<inode_t[]> inode_region(
      new inode_t[super.inode_region_len / sizeof(inode_t)]);
  readInodeRegion(&super, inode_region.get());
  memcpy(inode, &inode_region[inodeNumber], sizeof(inode_t));
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  inode_t inode;
  int ret = stat(inodeNumber, &inode);
  if (ret != 0)
    return -EINVALIDINODE;
  if (inode.size < size)
    return -EINVALIDSIZE;

  int write_offset = 0;
  int unread_len = size;
  for (auto direct_ptr : inode.direct) {
    if (unread_len == 0)
      break;
    int read_amt = unread_len > UFS_BLOCK_SIZE ? UFS_BLOCK_SIZE : unread_len;
    read_bytes(disk, direct_ptr * UFS_BLOCK_SIZE, read_amt, (char*)buffer + write_offset);
    unread_len -= read_amt;
    write_offset += read_amt;
  }
  return 0;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  inode_t parent;
  if (stat(parentInodeNumber, &parent))
    return -EINVALIDINODE;
  if (parent.type != UFS_DIRECTORY)
    return -EINVALIDTYPE;
  if (name.size() >= DIR_ENT_NAME_SIZE)
    return -EINVALIDNAME;

  int child_inum = lookup(parentInodeNumber, name);
  if (child_inum >= 0) {
    inode_t child;
    if (stat(child_inum, &child)) return -EINVALIDINODE;
    if (child.type == type)
      return child_inum;
    else 
      return -EINVALIDINODE;
  }
  else if (child_inum != -ENOTFOUND)
    return -EINVALIDINODE;

  super_t super;
  readSuperBlock(&super);
  unique_ptr<unsigned char[]> inode_bitmap(
      new unsigned char[UFS_BLOCK_SIZE * super.inode_bitmap_len]);
  readInodeBitmap(&super, inode_bitmap.get());

  int inum;
  char byte_buf;
  char mask;
  for (inum = 0; inum < super.num_inodes; inum++) {
    byte_buf = inode_bitmap[inum/8];
    mask = 1 << inum % 8;
    if ((byte_buf & mask) == 0) {
      break;
    }
  }
  if (inum >= super.num_inodes)
    return -ENOTENOUGHSPACE;

  disk->beginTransaction();

  inode_bitmap[inum/8] = byte_buf | mask;
  writeInodeBitmap(&super, inode_bitmap.get());

  inode_t new_file;
  new_file.size = 0;
  new_file.type = type;
  unique_ptr<inode_t[]> inode_region(
      new inode_t[UFS_BLOCK_SIZE * super.inode_region_len]);
  readInodeRegion(&super, inode_region.get());
  inode_region[inum] = new_file;
  writeInodeRegion(&super, inode_region.get());

  dir_ent_t new_entry;
  new_entry.inum = inum;
  strcpy(new_entry.name, name.c_str());
  vector<dir_ent_t> parent_buf;
  parent_buf.resize(parent.size / sizeof(dir_ent_t));
  if (read(parentInodeNumber, parent_buf.data(), parent.size)) {
    disk->rollback();
    return -EINVALIDINODE;
  }
  parent_buf.push_back(new_entry);
  if (write(parentInodeNumber, parent_buf.data(),
            parent_buf.size() * sizeof(dir_ent_t))) {
    disk->rollback();
    return -EINVALIDINODE;
  }

  disk->commit();
  return inum;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  disk->beginTransaction();
  super_t super;
  readSuperBlock(&super);
  inode_t inode;
  if (stat(inodeNumber, &inode)) {
    disk->rollback();
    return -EINVALIDINODE;
  }
  inode.size = size;

  // Allocate and free space
  size_t old_num_blks = bytes_to_blks(inode.size);
  size_t new_num_blks = bytes_to_blks(size);
  if (new_num_blks > DIRECT_PTRS) {
    disk->rollback();
    return -ENOTENOUGHSPACE;
  }
  if (old_num_blks < new_num_blks) {
    unique_ptr<unsigned char[]> data_bitmap(
        new unsigned char[super.data_bitmap_len * UFS_BLOCK_SIZE]);
    readDataBitmap(&super, data_bitmap.get());
    int blknum = 0;
    for (int direct_i = old_num_blks; direct_i < new_num_blks; direct_i++) {
      while (data_bitmap[blknum / 8] & (1 << (blknum % 8))) {
        blknum++;
        // bitmap is always larger than data region, no need for bound checks
        if ((blknum + 1) > super.data_region_len) {
          disk->rollback();
          return -ENOTENOUGHSPACE;
        }
      }
      inode.direct[direct_i] = blknum;
      data_bitmap[blknum / 8] |= 1 << (blknum % 8); // set
    }
    writeDataBitmap(&super, data_bitmap.get());
  }
  else if (old_num_blks > new_num_blks) {
    unique_ptr<unsigned char[]> data_bitmap(
        new unsigned char[super.data_bitmap_len * UFS_BLOCK_SIZE]);
    readDataBitmap(&super, data_bitmap.get());
    for (int direct_i = new_num_blks; direct_i < old_num_blks; direct_i++) {
      int blknum = inode.direct[direct_i];
      data_bitmap[blknum / 8] &= ~(1 << (blknum % 8));  // unset
    }
    writeDataBitmap(&super, data_bitmap.get());
  }

  // Write data region
  int write_offset = 0;
  int unwritten_nbytes = size;
  for (auto direct_ptr : inode.direct) {
    if (unwritten_nbytes == 0)
      break;
    int write_nbytes = unwritten_nbytes > UFS_BLOCK_SIZE ?
        UFS_BLOCK_SIZE : unwritten_nbytes;
    write_bytes(disk, direct_ptr * UFS_BLOCK_SIZE, write_nbytes, (char*)buffer + write_offset);
    unwritten_nbytes -= write_nbytes;
    write_offset += write_nbytes;
  }

  // Write inode
  unique_ptr<inode_t[]> inode_region(
      new inode_t[super.inode_region_len / sizeof(inode_t)]);
  readInodeRegion(&super, inode_region.get());
  inode_region[inodeNumber] = inode;
  writeInodeRegion(&super, inode_region.get());
  disk->commit();
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) { return 0; }
