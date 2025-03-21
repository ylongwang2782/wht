#ifndef MASTER_CFG_HPP
#define MASTER_CFG_HPP



//MSG
#define PCdataTransferMsg_DATA_QUEUE_SIZE 256
#define PCmanagerMsg_FORWARD_QUEUE_SIZE 10

//PCdataTransfer
#define PCdataTransfer_STACK_SIZE 4*1024

//PCinterface
#define PCinterface_FORWARD_QUEUE_TIMEOUT 5000
#define PCinterface_FORWARD_TIMEOUT 5000

#endif