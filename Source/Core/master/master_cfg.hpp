#ifndef MASTER_CFG_HPP
#define MASTER_CFG_HPP

// system
#define CONDUCTION_TEST_INTERVAL           5      // 导通检测时间间隔
#define CLIP_TEST_INTERVAL                 20     // 卡钉检测时间间隔
#define SYNC_TIMER_PERIOD_REDUNDANCY_TICKS 500    // 同步定时器冗余时间

// < MSG 任务通信消息 >---------------------------------------------------
// 上位机数据传输任务 <-> json解析任务：数据传输队列大小
#define PCdataTransferMsg_DATA_QUEUE_SIZE 2048

// json解析任务 <-> 从机管理任务：数据转发队列大小
#define PCmanagerMsg_FORWARD_QUEUE_SIZE 10

// 从机数据传输任务 <-> 从机管理任务：接受数据传输队列大小
#define ManagerDataTransferMsg_RXDATA_QUEUE_SIZE 2048

// < SlaveManager 从机管理任务 >----------------------------------------
// 栈大小
#define SlaveManager_STACK_SIZE 2 * 1024

// 发送数据入队超时
#define SlaveManager_TX_QUEUE_TIMEOUT 1000

// 发送超时
#define SlaveManager_TX_TIMEOUT 1000

// 从机回复超时
#define SlaveManager_RSP_TIMEOUT 1000

// 从机回复超时后重发次数
#define SlaveManager_TX_RETRY_TIMES 3

// 从机回复超时
#define SlaveManager_SlaveRSP_TIMEOUT 1000

// < ManagerDataTransfer 从机数据传输任务 >------------------------------
// 从机数据转发任务 栈大小
#define ManagerDataTransfer_STACK_SIZE 3 * 512

//==========================================================================================================

// < PCdataTransfer 上位机数据传输任务 >-----------------------------------
// 上位机数据传输任务 栈大小
#define PCdataTransfer_STACK_SIZE 3 * 512

// 上位机数据传输任务 发送缓冲区大小
#define PCdataTransfer_TX_BUFFER_SIZE 1024

// < PCinterface json解析任务 >------------------------------------------
// json解析任务 栈大小
#define PCinterface_STACK_SIZE 1500

// 从机管理任务 <-> json解析任务：转发数据入队超时时间
#define PCinterface_FORWARD_QUEUE_TIMEOUT 5000

// json解析任务 <-> 从机管理任务：数据转发超时时间
#define PCinterface_FORWARD_TIMEOUT                             \
    ((SlaveManager_TX_RETRY_TIMES + 1) *                        \
         (SlaveManager_TX_TIMEOUT + SlaveManager_RSP_TIMEOUT) + \
     5000)

// json解析任务 <-> 上位机数据传输任务：回复上位机时数据发送超时时间
#define PCinterface_RSP_TIMEOUT 1000

#endif