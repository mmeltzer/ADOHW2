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
#include "buffer_mgr.h"

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
  int numPages;         //size of the pool
  ReplacementStrategy strategy;
  void *mgmtData; // use this one to store the bookkeeping info your buffer
                  // manager needs for a buffer pool
} BM_BufferPool;

typedef struct BM_PageHandle {
  PageNumber pageNum;
  SM_PageHandle data;

  //add two features
  int pin_fix_count;   //can be increased or decreased
  int dirty;    //0 is clean, 1 is dirty.

} BM_PageHandle;

//the BM_mgmtData structure comprises:
//1, the pointer to the memory space that contains the pages.
//2, a pointer to a SM_FileHandle object that contains all the info about a file on disk.
typedef struct BM_mgmtData {

  BM_PageHandle *pages; //buffer initial address
  SM_FileHandle *fileHandle;

  //add two features, the number of pages in the pool that is occupied.
  int page_count;

  //add head and tail for FIFO queue
  int head;
  int tail;
	int read_count;
	int write_count;

  int *LRU_Order; //hold the accumulative number of LRU, use pointer to point to an array.

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
    BM_PageHandle *pages=(BM_PageHandle *)malloc(sizeof(BM_PageHandle) * numPages);

    int i, k;

    for (i=0;i<numPages;i++){

        //repeatedly create new memory space, and assign the space to BM_PageHandle's.
        SM_PageHandle page=(SM_PageHandle)malloc(PAGE_SIZE);

        //when using [], the pointer turns to object, so we use '.'
        pages[i].pageNum=-1;   //at beginning, set all page number to be -1.
        pages[i].data=page;

    }

    //assign the BM_PageHandle array to BM_mgmtData


    for(k=0;k<numPages;k++){

      //so, every page now is a pointer to a PageHandle object
      mgmtDataPool->pages[k]=pages[k];

    }

    //2, initialize a SM_FileHandle, and save it into BM_mgmtData
    SM_FileHandle *fileHandle=(SM_FileHandle *)malloc(sizeof(SM_FileHandle));

    openPageFile(pageFileName, fileHandle);

    mgmtDataPool->fileHandle=fileHandle;

    //3, initialize the int array that contains the dirty information, make them all 0.
    int *LRU_Order=(int *)malloc(sizeof(int)*numPages);

    mgmtDataPool->LRU_Order=LRU_Order;

    //4, page count
    mgmtDataPool->page_count=0;

    //5, head and tail for FIFO queue structure
    mgmtDataPool->head=-1;
    mgmtDataPool->tail=-1;

    //6, update the read_count and write_count
    mgmtDataPool->read_count = 0;
    mgmtDataPool->write_count = 0;

    //by now, the BM_mgmtData object has the memory space as well as all info about the file on disk.

    //at last, save all info into BM_BufferPool, including the BM_mgmtData just created.
    bm->pageFile=pageFileName;
    bm->numPages=numPages;
    bm->strategy=strategy;
    bm->mgmtData=mgmtDataPool;


    return RC_OK;


}


//how to completely shut down a buffer pool? modify it later
RC shutdownBufferPool(BM_BufferPool *const bm){

    return RC_OK;

}

//
RC forceFlushPool(BM_BufferPool *const bm){

  //write back after checking the dirty attribute and pin_fix_count attribute
  int i, page_count;

  //because mgmtData is void type in struct, so have to coerce it into BM_mgmtData it is not working if using 'page_count=bm->mgmtData->page_count;'
  BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);

  page_count=mgmtData->page_count;

  for(i=0;i<page_count;i++){

    if(mgmtData->pages[i].pin_fix_count==0 && mgmtData->pages[i].dirty==1){

      writeBlock(mgmtData->pages[i].pageNum, mgmtData->fileHandle, mgmtData->pages[i].data);

      //change the dirty into 0
      mgmtData->pages[i].dirty=0;

    }
  }

  return RC_OK;

}

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){

  //find the page in the buffer pool
  int position, k, page_count;

  position=0;

  BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);

  page_count=mgmtData->page_count;

  int exist=0;

  while(exist==1||position<page_count){

    k=mgmtData->pages[position].pageNum;

    if(page->pageNum==k){

      exist=1;

    }

    position++; //the position will be one more than real position when the loop ends

  }

  if(exist){
    position--;

    mgmtData->pages[position].dirty=1;

  }

  return RC_OK;

}


RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){

  //find the page in the buffer pool
  int position, k, page_count;

  position=0;

  BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);

  page_count=mgmtData->page_count;

  int exist=0;

  while(exist==1||position<page_count){

    k=mgmtData->pages[position].pageNum;

    if(page->pageNum==k){

      exist=1;

    }

    position++; //the position will be one more than real position when the loop ends

  }

  if(exist){
    position--;

    mgmtData->pages[position].pin_fix_count--;

  }


  return RC_OK;

}


//assume that input BM_PageHandle has the page number and the data.
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){


  BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);

  writeBlock(page->pageNum, mgmtData->fileHandle, page->data);

  //locate the position of the desired page in the buffer pool

  //find the page in the buffer pool
  int position, k, page_count;

  position=0;

  page_count=mgmtData->page_count;

  int exist=0;

  while(exist==1||position<page_count){

    k=mgmtData->pages[position].pageNum;

    if(page->pageNum==k){

      exist=true;

    }

    position++; //the position will be one more than real position when the loop ends

  }

  position--;

  //change the dirty to 0
  mgmtData->pages[position].dirty=0;

  return RC_OK;

}

//different strategies is implemented here.
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
	    const PageNumber pageNum){

  //check if the page is already in the buffer

  //get the position of this page
  int position, k, page_count;

  position=0;

  BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);

  page_count=mgmtData->page_count;

  int exist=0;

  while(exist==1||position<page_count){

    k=mgmtData->pages[position].pageNum;

    if(pageNum==k){

      exist=1;

    }

    position++; //the position will be one more than real position when the loop ends

  }


  if(exist){

    position--;

    //if it exists in buffer pool, then increase the fix count, and set the passed PageHandle
    mgmtData->pages[position].pin_fix_count++;

    //set the page number, content, and other info
    page->pageNum=pageNum;
    page->data=mgmtData->pages[position].data;
    page->pin_fix_count=mgmtData->pages[position].pin_fix_count;
    page->dirty=mgmtData->pages[position].dirty;

    return RC_OK;

  }

  //here, will be implementing different strategies
  else{


    //strategy 1, FIFO
    if(bm->strategy==RS_FIFO)
    {
      //if it doesn't exist, read the page into buffer pool, increase the fix count, and set the passed PageHandle
      //check the status of the queue

      //if the pages stored in the queue is less than the total capacity of the pool, add to the tail
      if(bm->numPages>page_count)
      {

        //if the queue has 0 elements, initialize the head to 0 and the tail to 0
        if(mgmtData->head == -1)
        {
          mgmtData->head = 0;
        }

        //increment tail, and assign new element to tail
        mgmtData->tail++;


        SM_PageHandle memPage;

        //read the page and store it at the position of tail
        memPage=mgmtData->pages[mgmtData->tail].data;

        readBlock(pageNum, mgmtData->fileHandle, memPage); //read a page from disk to this position
                                                              //in buffer pool
	      mgmtData->read_count++;

        //update other info, including the page number of this page, pin_fix_count, and dirty or not.
        mgmtData->pages[mgmtData->tail].pageNum=pageNum;
        mgmtData->pages[mgmtData->tail].pin_fix_count++;
        mgmtData->pages[mgmtData->tail].dirty=0;


        //set the features of PageHandle that has been passed in the method
        page->pageNum=pageNum;
        page->data=memPage;
        page->pin_fix_count=mgmtData->pages[mgmtData->tail].pin_fix_count;
        page->dirty=mgmtData->pages[mgmtData->tail].dirty;

        //increase the page_count in the mgmtData
        page_count++;

        mgmtData->page_count=page_count;

        return RC_OK;

      }

      //this is when the queue is full
      else
      {
        //write the new page at head position, and increment head and tail
        SM_PageHandle memPage;

        //read the page and store it at the position of head
        memPage=mgmtData->pages[mgmtData->head].data;

        readBlock(pageNum, mgmtData->fileHandle, memPage); //read a page from disk to this position
                                                              //in buffer pool
	      mgmtData->read_count++;

        mgmtData->head++;
        mgmtData->tail++;

        //if head reaches the end of the queue or the tail reaches the end of the queue, return them
        //back to the begining of the queue
        if(mgmtData->head==bm->numPages)
        {
          mgmtData->head=0;
        }

        if(mgmtData->tail==bm->numPages)
        {
          mgmtData->tail=0;
        }

        //update other info, including the page number of this page, pin_fix_count, and dirty or not.
        mgmtData->pages[mgmtData->tail].pageNum=pageNum;
        mgmtData->pages[mgmtData->tail].pin_fix_count++;
        mgmtData->pages[mgmtData->tail].dirty=0;


        //set the features of PageHandle that has been passed in the method
        page->pageNum=pageNum;
        page->data=mgmtData->pages[mgmtData->tail].data;
        page->pin_fix_count=mgmtData->pages[mgmtData->tail].pin_fix_count;
        page->dirty=mgmtData->pages[mgmtData->tail].dirty;

        //since the queue is full, no need to update the page_count

        return RC_OK;
      }

    }

    //strategy 2, LRU
    else if (bm->strategy==RS_LRU)
    {
      //if the queue is not full, add the element into the last position
      if(bm->numPages>page_count)
      {

        SM_PageHandle memPage;

        //read the page and store it at the position of tail
        memPage=mgmtData->pages[page_count].data;

        readBlock(pageNum, mgmtData->fileHandle, memPage); //read a page from disk to this position
                                                              //in buffer pool
	      mgmtData->read_count++;

        //increment the LRU_Order number for each element
        int i;

        for (i=0;i<page_count;i++){
          mgmtData->LRU_Order[i]++;
        }


        //update other info, including the page number of this page, pin_fix_count, and dirty or not.
        mgmtData->pages[mgmtData->tail].pageNum=pageNum;
        mgmtData->pages[mgmtData->tail].pin_fix_count++;
        mgmtData->pages[mgmtData->tail].dirty=0;


        //set the features of PageHandle that has been passed in the method
        page->pageNum=pageNum;
        page->data=mgmtData->pages[mgmtData->tail].data;
        page->pin_fix_count=mgmtData->pages[mgmtData->tail].pin_fix_count;
        page->dirty=mgmtData->pages[mgmtData->tail].dirty;

        //increase the page_count in the mgmtData
        page_count++;

        mgmtData->page_count=page_count;

        return RC_OK;
      }
      //if the queue is full, use LRU strategy
      else
      {
        //find the element with largest LRU_order number, and replace it with new element

        //find the position of this element
        int g, position, max_value;

        max_value=mgmtData->LRU_Order[0];

        for (g=0;g<page_count;g++){

          if(mgmtData->LRU_Order[g]>max_value)
          {
            max_value=mgmtData->LRU_Order[g];
            position=g;
          }
        }

        //increment the LRU_Order number for each element
        int f;

        for (f=0;f<page_count;f++){
          mgmtData->LRU_Order[f]++;
        }


        //replace the element at the position we got
        SM_PageHandle memPage;

        //read the page and store it at the position of tail
        memPage=mgmtData->pages[position].data;

        readBlock(pageNum, mgmtData->fileHandle, memPage); //read a page from disk to this position
                                                              //in buffer pool
	      mgmtData->read_count++;


        //update other info, including the page number of this page, pin_fix_count, and dirty or not.
        mgmtData->pages[mgmtData->tail].pageNum=pageNum;
        mgmtData->pages[mgmtData->tail].pin_fix_count++;
        mgmtData->pages[mgmtData->tail].dirty=0;


        //set the features of PageHandle that has been passed in the method
        page->pageNum=pageNum;
        page->data=mgmtData->pages[mgmtData->tail].data;
        page->pin_fix_count=mgmtData->pages[mgmtData->tail].pin_fix_count;
        page->dirty=mgmtData->pages[mgmtData->tail].dirty;

        //since the queue is full, no need to update the page_count

        return RC_OK;
      }
    }

    //strategy 3, CLOCK
    else{
      /* code */
    	return RC_OK;
    }
  }

}


PageNumber *getFrameContents (BM_BufferPool *const bm){

	int i;
	BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);

	int page_count=mgmtData->page_count;

	PageNumber *fcontents = malloc(sizeof(PageNumber) * bm->numPages);

	for(i=0;i<page_count;i++){

	    if(mgmtData->pages[i].pin_fix_count==0)
	    {
	      fcontents[i] = mgmtData->pages[i].pageNum;
	    }
	    else{
	    	fcontents[i] = NO_PAGE;
	    }
	}

	return fcontents;

}

bool *getDirtyFlags (BM_BufferPool *const bm){

	int i;
	BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);
	int page_count= mgmtData->page_count;

	bool *flags = malloc(sizeof(bool) * bm->numPages);

	for(i=0; i<page_count; i++){
		if(mgmtData->pages[i].dirty == 1){
			flags[i] = true;
		}
		else{
			flags[i] = false;
		}

	}

	return flags;
}

int *getFixCounts (BM_BufferPool *const bm){

	int i;
	BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);
	int page_count= mgmtData->page_count;

	int *fcounts = malloc(sizeof(int) * bm->numPages);

	for(i=0; i<page_count; i++)
	{
		fcounts[i]= mgmtData->pages[i].pin_fix_count;
	}

	return fcounts;

}

int getNumReadIO (BM_BufferPool *const bm){
	BM_mgmtData *mgmtData=(BM_mgmtData *)(bm->mgmtData);
	return mgmtData->read_count;
}


#endif
