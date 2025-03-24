#ifndef MASTER_DEF_HPP
#define MASTER_DEF_HPP
#include <cstddef>
#include <cstdint>
#include <iterator>

#include "EventCPP.h"
#include "Lock.h"
#include "MutexCPP.h"
#include "QueueCPP.h"
#include "SemaphoreCPP.h"
#include "master_cfg.hpp"
#include "portable.h"
enum CmdType : uint8_t {
    DEV_CONF = 0,
    DEV_MODE,
    DEV_RESET,
    DEV_CTRL,
    DEV_QUERY,
};

enum CtrlType : uint8_t {
    DEV_DISABLE = 0,
    DEV_ENABLE,
};

enum SysMode : uint8_t {
    CONDUCTION_TEST = 0,
    IMPEDANCE_TEST,
    CLIP_TEST

};

enum LockSta : uint8_t {
    UNLOCKED = 0,
    LOCKED,
};

struct CfgCmd {
    uint8_t id[4];
    uint16_t cond;
    uint16_t Z;
    uint16_t clip;
    uint16_t totalHarnessNum;
    uint16_t startHarnessNum;
};

struct ModeCmd {
    SysMode mode;
};

struct ResetCmd {
    uint8_t id[4];
    uint16_t clip;
    LockSta lock;
};

struct CtrlCmd {
    CtrlType ctrl;
};

struct QueryCmd {
    uint8_t id[4];
    uint8_t clip;
};

struct DataForward {
    CmdType type;
    CfgCmd cfg_cmd;
    ModeCmd mode_cmd;
    ResetCmd rst_cmd;
    CtrlCmd ctrl_cmd;
    QueryCmd query_cmd;
};

class Uint8ArrayIterator {
   public:
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;

    Uint8ArrayIterator(uint8_t* ptr) : ptr_(ptr) {}

    Uint8ArrayIterator& operator*() { return *this; }
    Uint8ArrayIterator& operator++() {
        ++ptr_;
        return *this;
    }
    Uint8ArrayIterator operator++(int) {
        Uint8ArrayIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    template <typename T>
    Uint8ArrayIterator& operator=(const T& value) {
        *ptr_ = static_cast<uint8_t>(value);
        return *this;
    }

   private:
    uint8_t* ptr_;
};

class ShareMem {
   public:
    ShareMem() : __mutex("mem"), __lock(__mutex) {}

    ShareMem(size_t wantedSize) : ShareMem() {
        shared_mem = (uint8_t*)pvPortMalloc(__size);
        __size = wantedSize;
    }

    ShareMem(uint8_t* __shared_mem, size_t wantedSize) : ShareMem() {
        shared_mem = __shared_mem;
        __size = wantedSize;
    }

    ShareMem(const ShareMem&) = delete;
    ~ShareMem() { vPortFree(shared_mem); }

    bool request() { return __lock.lock(); }
    bool request(uint32_t ms) { return __lock.lock(ms); }
    bool locked() { return __lock.locked(); }
    void release() { __lock.unlock(); }
    size_t size() { return __size; }

   private:
    Mutex __mutex;
    Lock __lock;
    size_t __size;

   public:
    uint8_t* shared_mem;
};

class PCdataTransferMsg {
   public:
    PCdataTransferMsg()
        : rx_done_sem("rx_done_sem"),
          tx_request_sem("tx_request_sem"),
          tx_done_sem("tx_done_sem"),
          tx_share_mem(PCdataTransfer_TX_BUFFER_SIZE) {}
    Queue<uint8_t, PCdataTransferMsg_DATA_QUEUE_SIZE> data_queue;
    BinarySemaphore rx_done_sem;

    BinarySemaphore tx_request_sem;
    BinarySemaphore tx_done_sem;
    ShareMem tx_share_mem;
};

#define FORWARD_SUCCESS_EVENT (EventBits_t)((EventBits_t)1 << 0)
#define CONFIG_SUCCESS_EVENT  (EventBits_t)((EventBits_t)1 << 1)
#define MODE_SUCCESS_EVENT    (EventBits_t)((EventBits_t)1 << 2)
#define RESET_SUCCESS_EVENT   (EventBits_t)((EventBits_t)1 << 3)
#define CTRL_SUCCESS_EVENT    (EventBits_t)((EventBits_t)1 << 4)
#define QUERY_SUCCESS_EVENT   (EventBits_t)((EventBits_t)1 << 5)
class PCmanagerMsg {
   public:
    Queue<DataForward, PCmanagerMsg_FORWARD_QUEUE_SIZE> data_forward_queue;
    EventGroup event;
};
#endif