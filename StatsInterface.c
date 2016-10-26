PageNumber *getFrameContents (BM_BufferPool *const bm){

	int i;
	int page_count= bm->mgmtData->page_count;

	PageNumber *fcontents = malloc(sizeof(PageNumber) * bm->numPages);


	for(i=0;i<page_count;i++){

	    if(bm->mgmtData->pages[i].pin_fix_count==0)
	    {
	      fcontents[i] = bm->mgmtData->pages[i].pageNum;
	    }
	    else{
	    	fcontents[i] = NO_PAGE;
	    }
	}

	return fcontents;

}

bool *getDirtyFlags (BM_BufferPool *const bm){

	int i;
	int page_count= bm->mgmtData->page_count;

	bool *flags = malloc(sizeof(bool) * bm->numPages);

	for(i=0; i<page_count; i++){
		if(bm->mgmtData->pages[i].dirty = 1){
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
	int page_count= bm->mgmtData->page_count;

	int *fcounts = malloc(sizeof(int) * bm->numPages);

	for(i=0; i<page_count; i++)
	{
		fcounts[i]= bm->mgmtData->pages[i].pin_fix_count;
	}

	return fcounts;

}

int getNumReadIO (BM_BufferPool *const bm){
	return bm->mgmtData->read_count;
}
