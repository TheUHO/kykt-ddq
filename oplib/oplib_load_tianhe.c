#include    <string.h>
#include    <stdlib.h>
#include 	"hthread_host.h"
#include	"mt_hybrid.h"

#include	"oplib.h"
#include	"error.h"

static int TIANHE_CLUSTER_COUNT = 4;

void	load_tianhe_init(){
    char *dat_file = FILE_DAT;  
    if (dat_file != NULL) {  
        printf("FILE_DAT is %s\n", dat_file);  
    } else {  
        printf("FILE_DAT not set.\n");  
    }  
    // char* dat_path = "/thfs1/home/lzz/hanbin/DSQ/ddq/compiled/tianhe_kernel.dat";
    
    int cluster_id;
    for(cluster_id=0; cluster_id<TIANHE_CLUSTER_COUNT; cluster_id++){
        
        if(hthread_dev_open(cluster_id) != MT_SUCCESS){
            ddq_error("load_tianhe_init() : hthread_dev_open(%d) failed!\n", cluster_id);
        }

        if(hthread_dat_load(cluster_id, dat_file) != MT_SUCCESS){
            ddq_error("load_dat : hthread_dat_load() with file '%s' failed in cluster %d!\n", dat_file, cluster_id);
        }
    }

}

void*	load_tianhe(char *funcname){

	void* res = mt_search_func(0, funcname);
	
    if (!res)
		ddq_error("load_tianhe() : mt_search_func() with function '%s' failed!\n", funcname);
	
    return res;
}

void	load_tianhe_finish(){
    int cluster_id;
    for(cluster_id=0; cluster_id<TIANHE_CLUSTER_COUNT; cluster_id++){
        if(hthread_dev_close(cluster_id) != MT_SUCCESS){
            ddq_error("load_tianhe_close() : hthread_dev_close(%d) failed!\n", cluster_id);
        }
    }
}