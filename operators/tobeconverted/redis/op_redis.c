//
// Created by fangx on 2023/7/12.
//

#include "op_redis.h"
#include "common_objs.h"
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static Linklist head;

void fn_connect(redisAsyncContext *c,void* reply,void* privdata){
    *((int*)privdata)=((redisReply*)reply)->type;
}

void fn_set(redisAsyncContext *c,void* reply,void* privdata){
}

void fn_getmem(redisAsyncContext *c,void* reply,void* privdata){
    redisReply* r=(redisReply*)reply;
    int len=r->len;
    Linklist pd=(Linklist)privdata;
    if(len>((obj_mem)(pd->obj))->bufsize){
        if(((obj_mem)(pd->obj))->p){
            free(((obj_mem)(pd->obj))->p);
        }
        ((obj_mem)(pd->obj))->p=malloc(len);
        ((obj_mem)(pd->obj))->bufsize=len;
        ((obj_mem)(pd->obj))->size=len;
        memcpy(((obj_mem)(pd->obj))->p,r->str,len);
    }
    else{
        memcpy(((obj_mem)(pd->obj))->p,r->str,len);
        ((obj_mem)(pd->obj))->size=len;
    }
    pd->flag=redis_done;
}

void fn_getvar(redisAsyncContext *c,void* reply,void* privdata){
    redisReply* r=(redisReply*)reply;
    Linklist pd=(Linklist)privdata;
    memcpy(pd->obj,r->str,r->len);
    pd->flag=redis_done;
}

task_ret op_redis_connect(void **inputs, void **outputs, void **attribute){
    char *address,*password;
    int port;
    address = (char*)(((obj_mem)inputs[0])->p);
    port = ((obj_var)inputs[1])->t_int;
    password = (char*)(((obj_mem)inputs[2])->p);

    redisAsyncContext *c = redisAsyncConnect(address, port);
    if (c==NULL||c->err) {
        if(c){
            printf("Connection error: %s\n", c->errstr);
            redisAsyncFree(c);
            c=NULL;
        }
        else{
            printf("Connection error: can't allocate redis context\n");
        }
        outputs[0]=NULL;
        return task_ret_error;//
    }
    int flag=0;
    redisAsyncCommand(c,fn_connect,&flag,"auth %s",password);
	redisAsyncHandleWrite(c);
    while(flag==0){
		redisAsyncHandleRead(c);
	}
    if(flag==REDIS_REPLY_ERROR){
        printf("Error: wrong password\n");
        return task_ret_error;//
    }
    outputs[0]=c;
    return task_ret_ok;
}

task_ret _command(void **inputs, void **outputs, redisCallbackFn *fn, void *privdata){
    redisAsyncContext *context = (redisAsyncContext*)inputs[0];
    char* fmt = (char*)inputs[1];
    char* format=(char*)malloc(strlen(fmt));
    obj_mem* args=(obj_mem*)malloc(3*sizeof(obj_mem));
    obj_mem tmp;
    _Bool flag[3],long_flag;
    memset(flag,0,sizeof(flag));
    int c=0,i=0;
    for(char* p=fmt; *p; p++){
        format[i++]=*p;
        long_flag=0;
        if ((*p)=='%') c++;
        else continue;
        format[i++]='b';
        if(c>3) {
            printf("Error: too much arguments\n");
            return task_ret_error;
        }
        p++;
        if((*p)=='l'){
			long_flag=1;
			p++;
		}
        switch (*p)
        {
        case 'b':
        case 's':
            args[c-1]=(obj_mem)inputs[c+1];
            break;

        case 'd':
        case 'u':
            tmp=obj_mem_new_size(4);
            tmp->size=4;
            memcpy(tmp->p,inputs[c+1],4);
            args[c-1]=tmp;
            flag[c-1]=1;
            break;

        case 'f':
            tmp=obj_mem_new_size(long_flag?8:4);
            tmp->size=long_flag?8:4;
            memcpy(tmp->p,inputs[c+1],long_flag?8:4);
            args[c-1]=tmp;
            flag[c-1]=1;
            break;
        
        default:
            printf("Error: unsupported argument type\n");
            return task_ret_error;
        }
    }
    switch (c)
    {
    case 0:
        redisAsyncCommand(context,fn,privdata,format);
        break;
    
    case 1:
        redisAsyncCommand(context,fn,privdata,format,args[0]->p,args[0]->size);
        break;

    case 2:
        redisAsyncCommand(context,fn,privdata,format,args[0]->p,args[0]->size,args[1]->p,args[1]->size);
        break;

    case 3:
        redisAsyncCommand(context,fn,privdata,format,args[0]->p,args[0]->size,args[1]->p,args[1]->size,args[2]->p,args[2]->size);
        break;
    }
    redisAsyncHandleWrite(context);
    for(;c>0;c--) if(flag[c-1]) obj_mem_del(args[c-1]);
    free(args);
    free(format);
    return task_ret_ok;
}

task_ret op_redis_command_set(void **inputs, void **outputs, void **attribute){ // %b %s %d %u %f %lf
    return _command(inputs,outputs,fn_set,NULL);
}

task_ret op_redis_command_getmem(void **inputs, void **outputs, void **attribute){
    Linklist p;
    for(p=head; p && p->obj != outputs[0]; p=p->next);
    redisAsyncContext *context = (redisAsyncContext*)inputs[0];
    if(p==NULL){
        Linklist q=(Linklist)malloc(sizeof(Node));
        q->obj=outputs[0];
        q->flag=redis_started;
        q->next=NULL;
        if(head==NULL){
            head=q;
            p=q;
        }
        else{
            p=head;
            while (p->next!=NULL) p=p->next;
            p->next=q;
            p=q;
        }
        _command(inputs,outputs,fn_getmem,q);
    }
    redisAsyncHandleRead(context);
    if(p->flag==redis_done){
        if(p==head){
            head=p->next;
        }
        else{
            Linklist q=head;
            while (q->next!=p) q=q->next;
            q->next=p->next;
        }
        free(p);
        return task_ret_ok;
    }
    return task_ret_again;
}

task_ret op_redis_command_getvar(void **inputs, void **outputs, void **attribute){
    Linklist p;
    for(p=head; p && p->obj != outputs[0]; p=p->next);
    redisAsyncContext *context = (redisAsyncContext*)inputs[0];
    if(p==NULL){
        Linklist q=(Linklist)malloc(sizeof(Node));
        q->obj=outputs[0];
        q->flag=redis_started;
        q->next=NULL;
        if(head==NULL){
            head=q;
            p=q;
        }
        else{
            p=head;
            while (p->next!=NULL) p=p->next;
            p->next=q;
            p=q;
        }
        _command(inputs,outputs,fn_getvar,q);
    }
    redisAsyncHandleRead(context);
    if(p->flag==redis_done){
        if(p==head){
            head=p->next;
        }
        else{
            Linklist q=head;
            while (q->next!=p) q=q->next;
            q->next=p->next;
        }
        free(p);
        return task_ret_ok;
    }
    return task_ret_again;
}
