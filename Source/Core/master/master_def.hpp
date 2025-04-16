#ifndef __MASTER_DEF_HPP
#define __MASTER_DEF_HPP
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include "EventCPP.h"
#include "Lock.h"
#include "MutexCPP.h"
#include "QueueCPP.h"
#include "SemaphoreCPP.h"
#include "master_cfg.hpp"
#include "portable.h"

// 迭代器-------------------------------------------------
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

// 共享内存-------------------------------------------------
class ShareMem {
   public:
    ShareMem()
        : __mutex("mem"),
          __lock(__mutex),
          __write_mtx("write_mtx"),
          __write_access(__write_mtx) {
        __write_access.unlock();
    }

    ShareMem(size_t wantedSize) : ShareMem() {
        __shared_mem = (uint8_t*)pvPortMalloc(wantedSize);
        if (__shared_mem == nullptr) {
            __init_success = false;
            __capacity = 0;
            return;
        } else {
            __init_success = true;
            __capacity = wantedSize;
        }
    }

    ShareMem(uint8_t* shared_mem, size_t wantedSize) : ShareMem() {
        __shared_mem = shared_mem;
        __init_success = true;
        __capacity = wantedSize;
    }

    ShareMem(const ShareMem&) = delete;
    ~ShareMem() { vPortFree(__shared_mem); }

    // 申请共享内存的写访问权限，防止数据在发送前被修改
    bool get_write_access(TickType_t wait = portMAX_DELAY) {
        return __write_access.lock(wait);
    }

    // 释放共享内存的写访问权限，等待数据转发完成后在释放写访问权限
    void release_write_access() { __write_access.unlock(); }
    
    bool write(const uint8_t* data, size_t size,
               TickType_t wait = portMAX_DELAY) {
        __size = 0;
        if (!__init_success) {
            return false;
        }
        if (data == nullptr) {
            return false;
        }
        if (size > __capacity) {
            return false;
        }

        // 当前无写访问权限
        if(!__write_access.lock()) {
            return false;
        }

        // 资源上锁，避免资源竞争
        __lock.lock(wait);
        __size = size;
        std::copy(data, data + size, __shared_mem);
        __lock.unlock();
        return true;
    }

    bool lock(TickType_t wait = portMAX_DELAY) {
        if (!__init_success) {
            return false;
        }
        return __lock.lock(wait);
    }
    void unlock() { __lock.unlock(); }
    uint8_t* get() { return __shared_mem; }
    size_t size() { return __size; }
    size_t capacity() { return __capacity; }

   private:
    Mutex __mutex;
    Lock __lock;
    Mutex __write_mtx;
    Lock __write_access;
    // BinarySemaphore __write_access;
    size_t __size;
    size_t __capacity;
    bool __init_success;
    uint8_t* __shared_mem;

   public:
};

// 配置指令-------------------------------------------------
struct CfgCmd {
    uint8_t id[4];
    uint16_t cond;
    uint16_t Z;

    bool clip_exist;
    uint8_t clip_mode;
    uint16_t clip_pin;

    uint16_t totalHarnessNum;
    uint16_t startHarnessNum;
    uint16_t slave_dev_num;
    bool is_last_dev;
};

// 模式指令-------------------------------------------------
enum SysMode : uint8_t {
    CONDUCTION_TEST = 0,
    IMPEDANCE_TEST,
    CLIP_TEST

};
struct ModeCmd {
    SysMode mode;
};

// 复位指令-------------------------------------------------
enum LockSta : uint8_t {
    UNLOCKED = 0,
    LOCKED,
};
struct ResetCmd {
    uint8_t id[4];
    uint16_t clip;
    LockSta lock;
};

// 控制指令-------------------------------------------------
enum CtrlType : uint8_t {
    DEV_DISABLE = 0,
    DEV_ENABLE,
};
struct CtrlCmd {
    CtrlType ctrl;
};

// 查询指令-------------------------------------------------
struct QueryCmd {
    uint8_t id[4];
    uint8_t clip;
};

// 状态回复-------------------------------------------------
enum StatusReply : uint8_t {
    STATUS_OK = 0,
    STATUS_ERROR,
};

// 转发数据-------------------------------------------------
enum CmdType : uint8_t {
    DEV_CONF = 0,
    DEV_MODE,
    DEV_RESET,
    DEV_CTRL,
    DEV_QUERY,
};
struct DataForward {
    CmdType type;
    CfgCmd cfg_cmd;
    ModeCmd mode_cmd;
    ResetCmd rst_cmd;
    CtrlCmd ctrl_cmd;
    QueryCmd query_cmd;
};

// 上位机数据传输任务 <-> json解析任务
// 消息结构---------------------------------------
class PCdataTransferMsg {
   public:
    PCdataTransferMsg()
        : rx_done_sem("rx_done_sem"),
          tx_request_sem("tx_request_sem"),
          tx_done_sem("tx_done_sem"),
          tx_share_mem(PCdataTransfer_TX_BUFFER_SIZE) {}
    Queue<uint8_t, PCdataTransferMsg_DATA_QUEUE_SIZE> rx_data_queue;
    BinarySemaphore rx_done_sem;
    BinarySemaphore tx_request_sem;
    BinarySemaphore tx_done_sem;
    ShareMem tx_share_mem;
};

// json解析任务 <-> 从机管理任务
// 消息结构---------------------------------------------
#define FORWARD_SUCCESS_EVENT (EventBits_t)((EventBits_t)1 << 0)
#define CONFIG_SUCCESS_EVENT  (EventBits_t)((EventBits_t)1 << 1)
#define MODE_SUCCESS_EVENT    (EventBits_t)((EventBits_t)1 << 2)
#define RESET_SUCCESS_EVENT   (EventBits_t)((EventBits_t)1 << 3)
#define CTRL_SUCCESS_EVENT    (EventBits_t)((EventBits_t)1 << 4)
#define QUERY_SUCCESS_EVENT   (EventBits_t)((EventBits_t)1 << 5)
class PCmanagerMsg {
   public:
    PCmanagerMsg(ShareMem& __upload_data, BinarySemaphore& __upload_request_sem,
                 BinarySemaphore& __upload_done_sem)
        : data_forward_queue("data_forward_queue"),
          upload_data(__upload_data),
          upload_request_sem(__upload_request_sem),
          upload_done_sem(__upload_done_sem) {}
    Queue<DataForward, PCmanagerMsg_FORWARD_QUEUE_SIZE> data_forward_queue;
    EventGroup event;
    ShareMem& upload_data;
    BinarySemaphore& upload_request_sem;
    BinarySemaphore& upload_done_sem;
};

// 从机数据传输任务 <-> 从机管理任务
// 消息结构-----------------------------------------------------
class ManagerDataTransferMsg {
   public:
    ManagerDataTransferMsg()
        : rx_done_sem("rx_done_sem"),
          tx_request_sem("tx_request_sem"),
          tx_done_sem("tx_done_sem"),

          tx_data_queue("manager_tx_data_queue"),
          rx_data_queue("manager_rx_data_queue") {}

   public:
    BinarySemaphore rx_done_sem;
    BinarySemaphore tx_request_sem;
    BinarySemaphore tx_done_sem;

    Queue<uint8_t, ManagerDataTransferMsg_RXDATA_QUEUE_SIZE> tx_data_queue;
    Queue<uint8_t, ManagerDataTransferMsg_RXDATA_QUEUE_SIZE> rx_data_queue;
};
#endif