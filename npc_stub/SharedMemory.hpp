#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <type_traits>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// ─────────────────────────────────────────────────────────────
//  Layout of the shared memory region
//
//  [ ShmHeader | write_buf[2048] | read_buf[2048] ]
//
//  "write" and "read" are from the perspective of the owner
//  (the process that created the segment).  A peer that
//  attaches sees the roles reversed: it writes into the
//  region this process reads from, and vice-versa.
// ─────────────────────────────────────────────────────────────

class SharedMemory {
public:
    static constexpr std::size_t BUFFER_SIZE = 2048;

private:
    // ── on-disk layout ──────────────────────────────────────
    struct ShmHeader {
        uint32_t magic;          // sanity / version tag
        uint32_t write_len;      // bytes currently valid in write_buf
        uint32_t read_len;       // bytes currently valid in read_buf
        uint8_t  _pad[4];        // keep buffers 8-byte aligned
    };

    static constexpr uint32_t MAGIC = 0x53484D5Fu;  // "SHM_"

    struct ShmLayout {
        ShmHeader header;
        uint8_t   write_buf[BUFFER_SIZE];
        uint8_t   read_buf[BUFFER_SIZE];
    };

    static constexpr std::size_t SHM_SIZE = sizeof(ShmLayout);

    // ── instance state ──────────────────────────────────────
    int         id_;
    std::string name_;      // e.g. "/shm_42"
    bool        owner_;     // true → we created it
    int         fd_;
    ShmLayout*  layout_;

public:
    // ── constructor / destructor ────────────────────────────

    /**
     * @param id  Numeric identifier.  Opens "/shm_<id>", creating it
     *            if it does not yet exist.
     */
    explicit SharedMemory(int id)
        : id_(id)
        , name_("/shm_" + std::to_string(id))
        , owner_(false)
        , fd_(-1)
        , layout_(nullptr)
    {
        open_or_create();
    }

    ~SharedMemory() { close(); }

    // Non-copyable, movable
    SharedMemory(const SharedMemory&)            = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedMemory(SharedMemory&& o) noexcept
        : id_(o.id_), name_(std::move(o.name_))
        , owner_(o.owner_), fd_(o.fd_), layout_(o.layout_)
    {
        o.fd_     = -1;
        o.layout_ = nullptr;
    }

    // ── public API ──────────────────────────────────────────

    /** Write up to BUFFER_SIZE bytes into the write buffer.
     *  @return number of bytes actually written. */
    std::size_t write(const void* data, std::size_t len)
    {
        if (!layout_) throw std::runtime_error("SharedMemory not open");
        len = std::min(len, static_cast<std::size_t>(BUFFER_SIZE));
        std::memcpy(layout_->write_buf, data, len);
        layout_->header.write_len = static_cast<uint32_t>(len);
        return len;
    }

    /** Read up to BUFFER_SIZE bytes from the read buffer.
     *  @return number of bytes placed into dst. */
    std::size_t read(void* dst, std::size_t max_len)
    {
        if (!layout_) throw std::runtime_error("SharedMemory not open");
        std::size_t available = layout_->header.read_len;
        std::size_t n = std::min(available, std::min(max_len, static_cast<std::size_t>(BUFFER_SIZE)));
        std::memcpy(dst, layout_->read_buf, n);
        return n;
    }

    /** Read raw bytes from the write buffer (inspect what this side wrote). */
    std::size_t read_write_buf(void* dst, std::size_t max_len)
    {
        if (!layout_) throw std::runtime_error("SharedMemory not open");
        std::size_t n = std::min(static_cast<std::size_t>(layout_->header.write_len),
                                 std::min(max_len, static_cast<std::size_t>(BUFFER_SIZE)));
        std::memcpy(dst, layout_->write_buf, n);
        return n;
    }

    /** Mark the write buffer as consumed (reset length to 0). */
    void clear_write_buf()
    {
        if (layout_) layout_->header.write_len = 0;
    }

    /** Write directly into the read buffer (so a peer can read it). */
    std::size_t write_to_read_buf(const void* data, std::size_t len)
    {
        if (!layout_) throw std::runtime_error("SharedMemory not open");
        len = std::min(len, static_cast<std::size_t>(BUFFER_SIZE));
        std::memcpy(layout_->read_buf, data, len);
        layout_->header.read_len = static_cast<uint32_t>(len);
        return len;
    }

    // Convenience typed overloads
    template<typename T>
    std::size_t write(const T& obj)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        return write(&obj, sizeof(T));
    }

    template<typename T>
    bool read(T& obj)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        return read(&obj, sizeof(T)) == sizeof(T);
    }

    // Accessors
    int         id()     const { return id_;     }
    bool        owner()  const { return owner_;  }
    std::size_t write_pending()   const { return layout_ ? layout_->header.write_len : 0; }
    std::size_t read_available()  const { return layout_ ? layout_->header.read_len  : 0; }

    /** Mark the write-buffer as consumed (set length to 0). */
    void clear_write()
    {
        if (layout_) layout_->header.write_len = 0;
    }

    /** Mark the read-buffer as consumed (set length to 0). */
    void clear_read()
    {
        if (layout_) layout_->header.read_len = 0;
    }

    /** Unlink the POSIX shared memory object (owner only). */
    void unlink()
    {
        if (owner_) ::shm_unlink(name_.c_str());
    }

    /** Detach and optionally unlink. */
    void close(bool do_unlink = false)
    {
        if (layout_) {
            ::munmap(layout_, SHM_SIZE);
            layout_ = nullptr;
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
        if (do_unlink) unlink();
    }

private:
    void open_or_create()
    {
        // Try to create exclusively first
        fd_ = ::shm_open(name_.c_str(), O_CREAT | O_EXCL | O_RDWR, 0660);
        if (fd_ >= 0) {
            owner_ = true;
            if (::ftruncate(fd_, static_cast<off_t>(SHM_SIZE)) < 0) {
                ::close(fd_);
                ::shm_unlink(name_.c_str());
                throw std::runtime_error("ftruncate failed: " + name_);
            }
        } else if (errno == EEXIST) {
            // Already exists — attach
            fd_ = ::shm_open(name_.c_str(), O_RDWR, 0660);
            if (fd_ < 0)
                throw std::runtime_error("shm_open(attach) failed: " + name_);
            owner_ = false;
        } else {
            throw std::runtime_error("shm_open failed: " + name_);
        }

        void* addr = ::mmap(nullptr, SHM_SIZE,
                            PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (addr == MAP_FAILED) {
            ::close(fd_);
            if (owner_) ::shm_unlink(name_.c_str());
            throw std::runtime_error("mmap failed: " + name_);
        }

        layout_ = static_cast<ShmLayout*>(addr);

        if (owner_) {
            // Zero-initialise and stamp magic
            std::memset(layout_, 0, SHM_SIZE);
            layout_->header.magic = MAGIC;
        } else {
            if (layout_->header.magic != MAGIC)
                throw std::runtime_error("Bad magic in shared segment: " + name_);
        }
    }
};