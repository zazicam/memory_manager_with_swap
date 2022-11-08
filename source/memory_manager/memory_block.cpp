#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>

#include "../utils/logger.hpp"
#include "memory_block.hpp"
#include "memory_pool.hpp"

// --------------------------------------------------------
// class MemoryBlock
// --------------------------------------------------------
MemoryBlock::MemoryBlock()
    : ptr_(nullptr), id_(0), capacity_(0), size_(0), locked_(false),
      pool_(nullptr), moved_(false) {}

MemoryBlock::MemoryBlock(void *ptr, SwapIdType id, size_t capacity, size_t size,
                         bool locked, MemoryPool *pool)
    : ptr_(ptr), id_(id), capacity_(capacity), size_(size), locked_(locked),
      pool_(pool), moved_(false) {}

void MemoryBlock::swap(MemoryBlock &other) {
    std::swap(ptr_, other.ptr_);
    std::swap(id_, other.id_);
    std::swap(capacity_, other.capacity_);
    std::swap(size_, other.size_);
    std::swap(locked_, other.locked_);
    std::swap(pool_, other.pool_);
    std::swap(moved_, other.moved_);
}

MemoryBlock::MemoryBlock(MemoryBlock &&other) { swap(other); }

MemoryBlock &MemoryBlock::operator=(MemoryBlock &&other) {
    swap(other);
    return *this;
}

size_t MemoryBlock::size() const { return size_; }

size_t MemoryBlock::capacity() const { return capacity_; }

void MemoryBlock::checkScopeError() const {
    if (moved_) {
        LOG_BEGIN
        logger << "ERROR: you can't use MemoryBlock in old scope after it was "
                  "moved to a new scope!"
               << std::endl;
        LOG_END
        exit(1);
    }
}

void MemoryBlock::lock() {
    checkScopeError();
    if (locked_ == false) {
        size_t blockIndex = pool_->blockIndexByAddress(ptr_);
        pool_->lockBlock(ptr_);
        pool_->swapMutex.lock();
        pool_->diskSwap->LoadBlockIntoRam(blockIndex, id_);
        pool_->swapMutex.unlock();
        locked_ = true;
    } else {
        LOG_BEGIN
        logger << "Info: lock() called for a locked block" << std::endl;
        LOG_END
    }
}

void MemoryBlock::unlock() {
    checkScopeError();
    if (locked_ == true) {
        pool_->unlockBlock(ptr_);
        locked_ = false;
    } else {
        LOG_BEGIN
        logger << "Info: unlock() called for an unlocked block" << std::endl;
        LOG_END
    }
}

bool MemoryBlock::isLocked() const {
    checkScopeError();
    return locked_;
}

void MemoryBlock::free() {
    checkScopeError();
    pool_->freeBlock(ptr_, id_);
}

void MemoryBlock::debugPrint() const {
    checkScopeError();
    LOG_BEGIN
    logger << "Block Info: " << std::endl;
    logger << "ptr: " << ptr_
           << ", blockIndex: " << pool_->blockIndexByAddress(ptr_) << std::endl;
    logger << "id: " << id_ << ", size: " << size_ << std::endl;
    logger << std::endl;
    LOG_END
}
