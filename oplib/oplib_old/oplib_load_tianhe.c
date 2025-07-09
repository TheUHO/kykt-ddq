#include	"oplib_private.h"
#include    "target.h"
#include    <string.h>
#include 	"hthread_host.h"
#include	"mt_hybrid.h"

static char* datPath;

void	oplib_load_tianheDat(char *datPath_){
	if (datPath){
        if(strcmp(datPath, datPath_) != 0){
            ddq_error("oplib_load_tianheDat : tianhe has already load a dat %s !", datPath);
        }
    }
	else
	{
        int cluster_id;
        for(cluster_id=0; cluster_id<TIANHE_CLUSTER_COUNT; cluster_id++){
            if(hthread_dat_load(cluster_id, datPath_) != MT_SUCCESS){
               ddq_error("oplib_load_tianheDat : hthread_dat_load() with file '%s' failed in cluster %d!\n", datPath_, cluster_id);
            }
        }

		datPath = malloc(sizeof(char) * (strlen(datPath_) + 1));
        strcpy(datPath, datPath_);
	}
}

void	oplib_load_tianheFunc(char *libname, char *funcname){
    char	*libtianhe3 = "%s/op_%s.dat";	// FIXME
	char	*funtianhe3 = "%s";
    char	*sl, *sf;
    sl = malloc(strlen(libtianhe3)+strlen(oplib_pref)+strlen(libname)+1);
    sf = malloc(strlen(funtianhe3)+strlen(funcname)+1);
    sprintf(sl, libtianhe3, oplib_pref, libname);
    sprintf(sf, funtianhe3, funcname);
    oplib_hash h;
    h = oplib_hash_pack(oplib_tianhe, sl, sf);
	h->p.f = (task_f_wildcard *)mt_search_func(0, sf);
	if (h->p.f == 0)
		ddq_error("oplib_load_tianheFunc() : mt_search_func() with function '%s' failed!\n", sf);
	oplib_hash_put(h);
}

void	oplib_load_tianheRun(char *libname, char *funcname){
    char	*libtianhe3 = "%s/op_%s.dat";	// FIXME
	char	*funtianhe3 = "%s";
    char	*sl, *sf;
    sl = malloc(strlen(libtianhe3)+strlen(oplib_pref)+strlen(libname)+1);
    sf = malloc(strlen(funtianhe3)+strlen(funcname)+1);
    sprintf(sl, libtianhe3, oplib_pref, libname);
    sprintf(sf, funtianhe3, funcname);
    oplib_hash h;
    h = oplib_hash_pack(oplib_tianhe, sl, sf);
	h->p.f = funtianhe3;
	oplib_hash_put(h);
}

void oplib_init_tianhe(){
    int cluster_id;
    for(cluster_id=0; cluster_id<TIANHE_CLUSTER_COUNT; cluster_id++){
        if(hthread_dev_open(cluster_id) != MT_SUCCESS){
            ddq_error("oplib_init_tianhe() : hthread_dev_open() failed!\n");
        }
    }
}
