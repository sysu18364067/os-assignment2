#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define FRAME_SIZE 256
#define TLB_SIZE 16            //TLB大小
#define PAGETABLE_SIZE 16      //页表大小
#define HashMaxSize 256         //使用一块32大小的哈希表来访问RLU缓存

/*main函数请跳转到380行*/

/*页码和偏移的掩码，用于从虚拟地址地区页号和偏移*/
uint8_t pageMask(uint32_t integer){
    uint32_t mask = 255;            //和255进行与操作能够提取出后8位
    integer = integer >> 8;         //如果要提取前八位，可以先右移
    uint8_t page = integer & mask;
    return page;
}

uint8_t biasMask(uint32_t integer){
    uint32_t mask = 255;
    uint8_t bias = integer & mask;
    return bias;
}

/*用于统计缺页率和TLB命中率的全局变量*/
int pageFault = 0;
int TLBhit = 0;

/*用FIFO时使用的TLB结构*/
typedef struct {
    int page;
    int frame;
} TLBelem;

/*用FIFO时使用的page结构*/
typedef struct {
    int page;
    int frame;
} page;

char* physicalMem[PAGETABLE_SIZE];   //物理内存
int physicalMemPtr = 0;              //物理内存已写位置的计数
page pageTable[PAGETABLE_SIZE];  //页表
TLBelem TLB[TLB_SIZE];           //TLB
int TLB_HEAD = 0;               //TLB头部
int PAGE_HEAD = 0;


/*基于LRU的TLB实现，使用双向链表和哈希表完成*/
/*双向链表结构*/
typedef struct DLinkedNode{
    int key;
    int frame;
    struct DLinkedNode* prev;
    struct DLinkedNode* next;
} DLinkedNode;
/*双向链表结构定义结束*/

/*哈希表结构定义开始*/
//为了区分，在函数一律使用key作为输入键值，key_表示哈希表键值
typedef size_t(*HashFunc)(int key);         //哈希函数
typedef enum {Empty, Valid, Invalid} HashStat;  //哈希表元素状态

//哈希表元素，包含一个键值、一个数据指针和一个状态
//其中，数据指针指向LRU缓存维护的双向链表中地元素，本身不存储值
//stat在Empty状态表示未使用，Valid为已使用并分配内存，Unvalid表示已删除并分配内存
typedef struct{
    int key;
    DLinkedNode* node;
    HashStat stat;
}HashElem;

//哈希表结构
typedef struct HashMap{
    HashElem data[HashMaxSize];  //哈希表存储单元
    size_t size;                 //有效的元素个数
    HashFunc hashfunc;           //哈希函数
}HashMap;

//哈希函数:key对哈希表大小求余
size_t HashFunction(int key){
    return key%HashMaxSize;
}

//简单的初始化
void HashMapInit(HashMap* ht){
    if(ht == NULL) return;
    ht->size = 0;
    ht->hashfunc = HashFunction;
    for(int i = 0; i < HashMaxSize; i++){
        ht->data[i].key = 0;
        ht->data[i].stat = Empty;
        ht->data[i].node = NULL;
    }
}

//哈希表插入操作
int HashMapInsert(HashMap* ht, int key, DLinkedNode* node){
    if(ht == NULL) return 0;                //哈希表无效
    if(ht->size >= HashMaxSize) return 0;   //哈希表满，插入操作无效
    int key_ = ht->hashfunc(key);            //获取哈希表下标
    while(1){                               //因为哈希表未满，肯定能找到位置
        if(ht->data[key_].key == key && ht->data[key_].stat == Valid)
		 	return 0;   //不接受重复key，当重复时插入失败
        if(ht->data[key_].stat != Valid){           //如果找到了哈希值开始的有效位置，则放置上去
        	//printf("set %d node\n", key_);
            ht->data[key_].key = key;
            ht->data[key_].node = node;
            ht->data[key_].stat = Valid;
            ht->size++;
            return 1;
        }
        key_ = ((key_+1) % HashMaxSize);            //到结尾就重新从表头找
    }
}

/*哈希表搜索key并返回哈希表下标key_*/
int HashMapFindKey(HashMap* ht, int key, int* key_)
{
    if (ht == NULL)                                             //哈希表无效
        return 0;
    for (int i = 0; i < HashMaxSize; i++)                       //查找
    {
        if (ht->data[i].key == key && ht->data[i].stat == Valid)
        {
            *key_ = i;                                          //存储下标，因为传入的是引用可以直接访问
            return 1;                                           //返回1表示成功找到key对应的哈希表项，其下标key_
        }
    }
    return 0;
}

//删除key节点
void HashRemove(HashMap* ht, int key){
    if(ht == NULL) return;                  //哈希表无效
    int key_ = 0;
    if(HashMapFindKey(ht,key,&key_) == 0) return;   //找不到key在哈希表中对应的下标key_
    else{
        ht->data[key_].stat = Invalid;              //将该节点置为无效，但不删除空间,因为空间是链表维护的
    	ht->size--;
	}
}

///哈希表为空？
int HashEmpty(HashMap* hm){
    if(hm==NULL) return 0;
    return hm->size == 0;
}

//求哈希表的大小
size_t HashSize(HashMap* hm){
    if(hm == NULL) return 0;
    else return hm->size;
}

/*哈希表结构定义结束*/

/*LRU缓存定义*/
/*LRU缓存由一张哈希表和一个双向链表组成*/
/*哈希表主要负责查找，链表维护了一个以节点最近*/
/*访问顺序排列的队列，并管理空间分配*/
typedef struct LRUCache{
    HashMap cache;
    int size;
    int capacity;
    DLinkedNode head;
    DLinkedNode tail;
}LRUCache;

/*基于双向链表和哈希表的LRU结构，初始化容积*/
/*其中，双向链表存储了包含页码到帧码的节点*/
/*而哈希表将页码映射为指向对应节点的指针*/
void LRUinit(LRUCache* lc, int cap){
    HashMapInit(&(lc->cache));
    lc->size = 0;
    lc->capacity = cap;
    lc->head.next = &(lc->tail);
    lc->tail.prev = &(lc->head);
    lc->head.prev = NULL;
    lc->tail.next = NULL;
}

/*将一个节点添加到双向链表头，同时更新哈希表*/
void LRUaddNode(LRUCache* lc, DLinkedNode* node){
    if(lc->size >= lc->capacity) return;
    node->prev = &(lc->head);
    node->next = lc->head.next;
    node->next->prev = node;
    lc->head.next = node;
    lc->size++;
    HashMapInsert(&(lc->cache), node->key, node);
}

/*LRU移除节点，但是不删除节点空间。空间操作交给get和put执行*/
void LRUremoveNode(LRUCache* lc, DLinkedNode* node){
    if(lc->size <= 0) return;
    DLinkedNode* prev = node->prev;
    DLinkedNode* next = node->next;
    prev->next = next;
    next->prev = prev;
    lc->size--;
}

/*移到表头*/
void LRUmoveToHead(LRUCache* lc, DLinkedNode* node){
    LRUremoveNode(lc, node);
    node->next = lc->head.next;
    node->prev = &(lc->head);
    node->next->prev = node;
    lc->head.next = node;
}

void LRUremoveTail(LRUCache* lc){
	DLinkedNode* temp = lc->tail.prev;
    LRUremoveNode(lc, lc->tail.prev);
    free(temp);
}

/*LRU释放内存*/
void LRUdeleteAll(LRUCache* lc){
    DLinkedNode* ptr;
    /*遍历整个链表，释放所有节点*/
    for(ptr=lc->head.next; ptr!=&(lc->tail); ){
        ptr = ptr->next;
        free(ptr->prev);
    }
}

void LRUprintAll(LRUCache* lc){
    DLinkedNode* ptr;
    /*遍历整个链表，释放所有节点*/
    printf("========DLinked==========\n");
    for(ptr=&(lc->head); ptr->next!=&(lc->tail); ){
        ptr = ptr->next;
        printf("%d %d\n", ptr->key, ptr->frame);
    }
    printf("========HashMap========\n");
    printf("hash size: %d\n", (int)(lc->cache.size));
    for(int i = 0;i < HashMaxSize; i++){
    	printf("key: %d, valid: %d\n", lc->cache.data[i].key, lc->cache.data[i].stat);
	}
    printf("=======================\n");
}

/*put函数负责向LRU中加入一个条目*/
/*如果key存在于LRU缓存中，则将其移到表头*/
/*否则，如果缓存未满，加入表头，如果已满，删除表尾并加入表头*/
void put(LRUCache* lc, int key, int value){
    int key_;
    if(HashMapFindKey(&(lc->cache), key, &key_)){      //如果哈希表有key元素
        LRUremoveNode(lc, (lc->cache.data[key_].node));         //删除key元素，并稍后在表头重加。由于其未从哈希表删除，我们可以通过下标访问被remove但未delete的节点
    	//printf("find a same key, try to move it to head\n");
	}
    if(lc->size == lc->capacity) {                  //如果已满，删除尾部元素
    	//printf("FULL, deleteTail (%d, %d)\n", lc->tail.prev->key, lc->tail.prev->frame);
        HashRemove(&(lc->cache), lc->tail.prev->key);
        LRUremoveTail(lc);
    }
    //printf("Adding newNode\n");
	DLinkedNode* newNode = (DLinkedNode*)malloc(sizeof(DLinkedNode));   //建立新节点
    newNode->key = key; newNode->frame = value;                         //节点的键值分别是虚拟内存页和帧页码
    LRUaddNode(lc, newNode);                                            //将节点添加到LRU缓存
	HashMapInsert(&(lc->cache), key, newNode);
}

/*get函数负责从哈希表中找到LRU链表节点，*/
int get(LRUCache* lc, int key){
    int key_;
    if(HashMapFindKey(&(lc->cache), key, &key_)==0) return -1;    //在哈希表中找到key对应哈希表下标
    DLinkedNode* tempNode = lc->cache.data[key_].node;                //备份该节点
    LRUremoveNode(lc, tempNode);                     //删除该节点
    LRUaddNode(lc, tempNode);                               //然后将其提到最前面
    return tempNode->frame;                                 //返回其值
}

/*LRU缓存定义结束*/

/*FIFO*/
/*对于FIFO，使用普通的数组，以队列方式组织数据*/

void freePhyMem();                  //释放所有物理内存
int searchTLB(int);
int searchPageTable(int);
void TLBInit();
void TLB_update_FIFO(int, int);
int searchPageTable(int);
void pageTableInit();

void freePhyMem(){
    for(int i = 0; i < PAGETABLE_SIZE; i++){
        free(physicalMem[i]);
    }
}

void TLBInit(){
    memset(TLB, -1, sizeof(TLB));
}

int searchTLB(int page){
    for(int i = 0; i < TLB_SIZE; i++){
        if(TLB[i].page == page){
            //printf("TLB hit! on %d\n", i);
            return TLB[i].frame;
        }
    }
    //printf("TLB miss!\n");
    return -1;
}

void TLB_update_FIFO(int page, int frame){
    //printf("TLB_update_FIFO\n");
    TLB[TLB_HEAD].page = page;
    TLB[TLB_HEAD].frame = frame;
    TLB_HEAD = (TLB_HEAD+1)%TLB_SIZE;
}

void pageTableInit(){
    memset(pageTable, -1, sizeof(pageTable));
}

int searchPageTable(int page){
    for(int i = 0; i < PAGETABLE_SIZE; i++){
        if(pageTable[i].page == page){
            //printf("page hit!\n");
            return pageTable[i].frame;
        }
    }
    //printf("page miss!\n");
    return -1;
}

void pageTable_update_FIFO(int page, int frame){
    int swapOut;
    pageTable[PAGE_HEAD].page = page;
    pageTable[PAGE_HEAD].frame = frame;
    PAGE_HEAD = (PAGE_HEAD+1)%PAGETABLE_SIZE;
}

int question1(int argc, char* argv[])
int question2(int argc, char* argv[])

int main(int argc, char* argv[]){
    question1(argc, argv);
    return 0;
}


/*第一题实现*/
int question1(int argc, char* argv[]){
    int ch = getopt(argc, argv, "r:");
    void (*updateTLB)(int, int);
    void (*updatePT)(int, int);
    if(ch == -1){               //没有p参数，默认使用FIFO
        ch = 'r';
        optarg = "LRU";
    }
    FILE* logicAddr = fopen(argv[2], "rb");
    uint32_t buffer;                            //读取address文件用缓冲区
    FILE* backingStore = fopen(argv[1], "rb");
    if(ch == 'r'){
        if(strcmp(optarg, "FIFO") == 0){
            updateTLB = TLB_update_FIFO;
            updatePT = pageTable_update_FIFO;

            //初始化物理内存、页表和TLB
            for(int i = 0; i < PAGETABLE_SIZE; i++) physicalMem[i] = NULL;
            pageTableInit();
            TLBInit();
            memset(TLB, -1, sizeof(TLBelem)*TLB_SIZE);                                     

            //遍历address文件
            for(int i = 0; i < 1000; i++){
                fscanf(logicAddr, "%d", &buffer);
                uint8_t page = pageMask(buffer);                //获取页
                uint8_t bias = biasMask(buffer);                //获取偏移
                char value;
                int frame;
                if((frame = searchTLB(page)) != -1){            //查找TLB，如果找到了则TLB命中，输出页帧号
                    TLBhit++;
                    printf("Virtual address: %d Physical address: %d Value: %d\n",\
                        buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                }
                else{
                    if((frame = searchPageTable(page)) == -1){              //未命中TLB则搜索页表，如果找不到
                        pageFault++;
                        fseek(backingStore, FRAME_SIZE*page, 0);            //在后备存储中获取对应页
                        char* newMem = (char*)malloc(FRAME_SIZE);           //分配一帧内存空间
                        fread(newMem, 1, FRAME_SIZE, backingStore);         //将后背存储读入到内存
                        if(physicalMem[physicalMemPtr] != NULL) free(physicalMem[physicalMemPtr]);      //将物理内存放置到physicalMem数组中
                        physicalMem[physicalMemPtr] = newMem;
                        frame = physicalMemPtr;                                           
                        updatePT(page, physicalMemPtr);                                                 //FIFO更新页表
                        printf("Virtual address: %d Physical address: %d Value: %d\n",\
                            buffer, (physicalMemPtr<<8)+bias, physicalMem[physicalMemPtr][bias]);
                        physicalMemPtr = (physicalMemPtr+1)%PAGETABLE_SIZE;
                    }
                    else{
                        printf("Virtual address: %d Physical address: %d Value: %d\n",\
                            buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                    }
                    updateTLB(page, frame);
                }
            }
        }
        else if(strcmp(optarg, "LRU") == 0){
            LRUCache pageTable;
            LRUCache TLB;
            LRUinit(&TLB, TLB_SIZE);
            LRUinit(&pageTable, PAGETABLE_SIZE);
            for(int i = 0; i < PAGETABLE_SIZE; i++) physicalMem[i] = NULL;
            for(int i = 0; i < 1000; i++){
                fscanf(logicAddr, "%d", &buffer);
                uint8_t page = pageMask(buffer);                //获取页
                uint8_t bias = biasMask(buffer);                //获取偏移
                int frame;
                if((frame = get(&TLB, page)) != -1){          //frame不为-1说明找到
                    printf("Virtual address: %d Physical address: %d Value: %d\n",\
                        buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                    TLBhit++;
                }
                else{
                    if((frame = get(&pageTable, page)) != -1){     //在页表中查找，如果找到则输出
                        printf("Virtual address: %d Physical address: %d Value: %d\n",\
                            buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                            //printf("\n=========(%d, %d)=========\n", page, frame);
                    }
                    else{         
                        pageFault++;
                        fseek(backingStore, FRAME_SIZE*page, 0);            //在后备存储中获取对应页
                        char* newMem = (char*)malloc(FRAME_SIZE);           //分配一帧内存空间
                        fread(newMem, 1, FRAME_SIZE, backingStore);         //将后背存储读入到内存
                        if(physicalMem[physicalMemPtr] != NULL) free(physicalMem[physicalMemPtr]);      //将物理内存放置到physicalMem数组中
                        physicalMem[physicalMemPtr] = newMem;   
                        frame = physicalMemPtr;                                            //在页表中找不到，载入到内存。
                        put(&(pageTable), page, frame);
                        printf("Virtual address: %d Physical address: %d Value: %d\n",\
                            buffer, (frame<<8)+bias, physicalMem[physicalMemPtr][bias]);
                        physicalMemPtr = (physicalMemPtr+1)%PAGETABLE_SIZE;
                        //LRUprintAll(&pageTable);
                    }
                    put(&(TLB), page, frame);
                }
            }
            LRUdeleteAll(&pageTable);LRUdeleteAll(&TLB);
        }
        fclose(backingStore);
        fclose(logicAddr);
        freePhyMem();
        printf("TLBHitRate: %f%%\npageFaultRate: %f%%\n", TLBhit/10.0, pageFault/10.0);
    }
}

int question2(int argc, char* argv[]){
    int ch = getopt(argc, argv, "r:");
    void (*updateTLB)(int, int);
    void (*updatePT)(int, int);
    if(ch == -1){               //没有p参数，默认使用FIFO
        ch = 'r';
        optarg = "LRU";
    }
    FILE* logicAddr = fopen(argv[2], "rb");
    uint32_t buffer;                            //读取address文件用缓冲区
    FILE* backingStore = fopen(argv[1], "rb");
    if(ch == 'r'){
        if(strcmp(optarg, "FIFO") == 0){
            updateTLB = TLB_update_FIFO;
            updatePT = pageTable_update_FIFO;

            //初始化物理内存、页表和TLB
            for(int i = 0; i < PAGETABLE_SIZE; i++) physicalMem[i] = NULL;
            pageTableInit();
            TLBInit();
            memset(TLB, -1, sizeof(TLBelem)*TLB_SIZE);                                     

            //遍历address文件
            for(int i = 0; i < 1000; i++){
                fscanf(logicAddr, "%d", &buffer);
                int page = buffer;                          //获取页
                char value;
                int frame;
                if((frame = searchTLB(page)) != -1){            //查找TLB，如果找到了则TLB命中，输出页帧号
                    TLBhit++;
                    //printf("Virtual address: %d Physical address: %d Value: %d\n",\
                    //    buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                }
                else{
                    if((frame = searchPageTable(page)) == -1){              //未命中TLB则搜索页表，如果找不到
                        pageFault++;
                        fseek(backingStore, FRAME_SIZE*page, 0);            //在后备存储中获取对应页
                        char* newMem = (char*)malloc(FRAME_SIZE);           //分配一帧内存空间
                        fread(newMem, 1, FRAME_SIZE, backingStore);         //将后背存储读入到内存
                        if(physicalMem[physicalMemPtr] != NULL) free(physicalMem[physicalMemPtr]);      //将物理内存放置到physicalMem数组中
                        physicalMem[physicalMemPtr] = newMem;
                        frame = physicalMemPtr;                                           
                        updatePT(page, physicalMemPtr);                                                 //FIFO更新页表
                    //    printf("Virtual address: %d Physical address: %d Value: %d\n",\
                    //        buffer, (physicalMemPtr<<8)+bias, physicalMem[physicalMemPtr][bias]);
                        physicalMemPtr = (physicalMemPtr+1)%PAGETABLE_SIZE;
                    }
                    else{
                    //    printf("Virtual address: %d Physical address: %d Value: %d\n",\
                    //        buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                    }
                    updateTLB(page, frame);
                }
            }
        }
        else if(strcmp(optarg, "LRU") == 0){
            LRUCache pageTable;
            LRUCache TLB;
            LRUinit(&TLB, TLB_SIZE);
            LRUinit(&pageTable, PAGETABLE_SIZE);
            for(int i = 0; i < PAGETABLE_SIZE; i++) physicalMem[i] = NULL;
            for(int i = 0; i < 1000; i++){
                fscanf(logicAddr, "%d", &buffer);
                int page = buffer;                //获取页
                int frame;
                if((frame = get(&TLB, page)) != -1){          //frame不为-1说明找到
                    //printf("Virtual address: %d Physical address: %d Value: %d\n",\
                    //    buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                    TLBhit++;
                }
                else{
                    if((frame = get(&pageTable, page)) != -1){     //在页表中查找，如果找到则输出
                        //printf("Virtual address: %d Physical address: %d Value: %d\n",\
                        //    buffer, (frame<<8)+bias, physicalMem[frame][bias]);
                            //printf("\n=========(%d, %d)=========\n", page, frame);
                    }
                    else{         
                        pageFault++;
                        fseek(backingStore, FRAME_SIZE*page, 0);            //在后备存储中获取对应页
                        char* newMem = (char*)malloc(FRAME_SIZE);           //分配一帧内存空间
                        fread(newMem, 1, FRAME_SIZE, backingStore);         //将后背存储读入到内存
                        if(physicalMem[physicalMemPtr] != NULL) free(physicalMem[physicalMemPtr]);      //将物理内存放置到physicalMem数组中
                        physicalMem[physicalMemPtr] = newMem;   
                        frame = physicalMemPtr;                                            //在页表中找不到，载入到内存。
                        put(&(pageTable), page, frame);
                        //printf("Virtual address: %d Physical address: %d Value: %d\n",\
                        //    buffer, (frame<<8)+bias, physicalMem[physicalMemPtr][bias]);
                        physicalMemPtr = (physicalMemPtr+1)%PAGETABLE_SIZE;
                        //LRUprintAll(&pageTable);
                    }
                    put(&(TLB), page, frame);
                }
            }
            LRUdeleteAll(&pageTable);LRUdeleteAll(&TLB);
        }
        fclose(backingStore);
        fclose(logicAddr);
        freePhyMem();
        printf("TLBHitRate: %f%%\npageFaultRate: %f%%\n", TLBhit/10.0, pageFault/10.0);
    }
}
