#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include <stdio.h>
#include <stdlib.h>
#include "dberror.h"
#include "storage_mgr.h"
#include <string.h>

// Include bool DT
#include "dt.h"

/*
enum flag { const1, const2, ..., constN };
Here, name of the enumeration is flag.
And, const1, const2,...., constN are values of type flag.
By default, const1 is 0, const2  is 1 and so on. You can change default values of enum elements during 
declaration (if necessary).
*/


// Replacement Strategies
typedef enum ReplacementStrategy {
  RS_FIFO = 0,
  RS_LRU = 1,
  RS_CLOCK = 2,
  RS_LFU = 3,
  RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

typedef struct BM_BufferPool {
  char *pageFile;
  int numPages;
  ReplacementStrategy strategy;
  void *mgmtData; // use this one to store the bookkeeping info your buffer 
                  // manager needs for a buffer pool
} BM_BufferPool;

typedef struct BM_PageHandle {
  PageNumber pageNum;
  char *data;
} BM_PageHandle;

//the BM_mgmtData structure comprises:
//1, the pointer to the memory space that contains the pages.
//2, a pointer to a SM_FileHandle object that contains all the info about a file on disk.
typedef struct BM_mgmtData {

  BM_PageHandle *pages[];
  SM_FileHandle *fileHandle;
  int pin[];
  int dirty[];
  int LRU_order[]; //hold the order of LRU

} BM_mgmtData;

// convenience macros
#define MAKE_POOL()					\
  ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
  ((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))


/*
char * const a;
means that the pointer is constant and immutable but the pointed data is not.

const char * a;
means that the pointed data cannot be written to using the pointer a.

const char * const
which is a pointer that you cannot change, to a char (or multiple chars) that you cannot change. 
*/



// Buffer Manager Interface Pool Handling

// a little confused about the stratData, what does it do?
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData){

    //initialize the BM_mgmtData
    //1, create an BM_mgmtData object
    BM_mgmtData *mgmtDataPool=(BM_mgmtData *)malloc(sizeof(BM_mgmtData));

    //create BM_PageHandle array
    BM_PageHandle *pages=(BM_PageHandle *)malloc(sizeof(BM_PageHandle) ＊ numPages);

    int i, k;

    for (i=0;i<numPages;i++){

        //repeatedly create new memory space, and assign the space to BM_PageHandle's.
        SM_PageHandle *page=(SM_PageHandle *)malloc(PAGE_SIZE);

        //when to use '.', when to use '->'?
        pages[i].PageNumber=i;
        pages[i].data=page;

    }

    //assign the BM_PageHandle array to BM_mgmtData
    

    for(k=0;k<numPages;k++){

      mgmtDataPool->pages[k]=&pages[k];

    }

    //2, initialize a SM_FileHandle, and save it into BM_mgmtData
    SM_FileHandle *fileHandle=(SM_FileHandle *)malloc(sizeof(SM_FileHandle));

    openPageFile(pageFileName, fileHandle);

    mgmtDataPool->fileHandle=fileHandle;

    //3 initialize the int array that contains the pin information, make them all 0.
    int pin[numPages]={0};
    memcpy(mgmtDataPool->pin, pin, sizeof(pin));

    //4, initialize the int array that contains the dirty information, make them all 0.
    int dirty[numPages]={0};
    memcpy(mgmtDataPool->dirty, dirty, sizeof(dirty));

    //4, initialize the int array that contains the dirty information, make them all 0.
    int LRU_order[numPages]={0};
    memcpy(mgmtDataPool->LRU_order, LRU_order, sizeof(LRU_order));

    //by now, the BM_mgmtData object has the memory space as well as all info about the file on disk.

    //at last, save all info into BM_BufferPool, including the BM_mgmtData just created.
    bm->pageFile=pageFileName;
    bm->numPages=numPages;
    bm->strategy=strategy;
    bm->mgmtData=mgmtDataPool;


}


//how to completely shut down a buffer pool?
RC shutdownBufferPool(BM_BufferPool *const bm){

    //remove the memory space for containing the pages
    free(bm->mgmtData->memPool);

    //close the file.
    close(bm->mgmtData->fileHandle->mgmInfo->fp);

}

//
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
bool *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);

#endif
