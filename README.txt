Team members: Marty Meltzer (mmeltze3), Sachet Misra (smisra8), Guoxuan Hao (ghao2), 
Manojkumar Gaddam (Leader; email ID is mgaddam)

File List: buffer_mgr.c, buffer_mgr.h, buffer_mgr_stat.c, buffer_mgr_stat.h, dberror.c, dberror.h, dt.h, makefile,
		storage_mgr.c, storage_mgr.h, test_assign2_1.c, test_helper.h

No additional functions or error codes were created. 

The main data structure used was BM_mgmtData. Here is the code of the data structure:

typedef struct BM_mgmtData {

  BM_PageHandle *pages; //buffer initial address
  SM_FileHandle *fileHandle;

  //add two features, the number of pages in the pool that is occupied.
  int page_count;

  //add the count of pages read from disk and count of pages written to the page file
  int read_count;
  int write_count;

  int *LRU_Order; //hold the accumulative number of LRU, use pointer to point to an array.

} BM_mgmtData;