
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"error.h"
#include	"std_types.h"


#ifdef	enable_processor_cuda
// TODO: include header files
#endif


/*
 * 与拷贝有关的代码
 */

static void		prepare_default(obj_mem po, int_t size)
{
	if (po->bufsize < size)
	{
		if (po->p)
			free(po->p);		// FIXME: 原来的数据直接扔掉是否合理？
		po->p = malloc(size);
		po->bufsize = size;
	}
	po->size = size;
}

static void		copy_default(obj_mem po, obj_mem pi)
{
	memcpy(po->p, pi->p, po->size);
}


static void		copy_file(obj_mem po, obj_mem pi)
{
	FILE	*fp;
	int_t	size, s;

	if (pi->type == obj_file_local && po->type == obj_mem_default)
	{
		fp = fopen(pi->p, "rb");
		if (!fp)
			ddq_error("std/obj/copy : Failed opening file '%s' for reading.\n", (char *)pi->p);

		s = fseek(fp, 0, SEEK_END);
		if (s)
			ddq_warning("std/obj/copy : Something is wrong when looking for the end of file '%s'.\n", (char *)pi->p);
		size = ftell(fp);
		if (size < 0)
			ddq_warning("std/obj/copy : Something is wrong when calculating the size of file '%s'.\n", (char *)pi->p);
		s = fseek(fp, 0, SEEK_SET);
		if (s)
			ddq_warning("std/obj/copy : Something is wrong when seek back to the beginning of file '%s'.\n", (char *)pi->p);

		prepare_default(po, size);

		po->size = fread(po->p, 1, size, fp);
		if (po->size != size)
			ddq_warning("std/obj/copy : Partially read (%ld/%ld) bytes from file '%s'.\n", po->size, size, (char *)pi->p);

		s = fclose(fp);
		if (s)
			ddq_warning("std/obj/copy : Something is wrong when closing file '%s'.\n", (char *)pi->p);
	}
	else if (pi->type == obj_mem_default && po->type == obj_file_local)
	{
		fp = fopen(po->p, "wb");
		if (!fp)
			ddq_error("std/obj/copy : Failed opening file '%s' for writing.\n", (char *)po->p);

		s = fwrite(pi->p, 1, pi->size, fp);
		if (s != pi->size)
			ddq_warning("std/obj/copy : Partially written (%ld/%ld) bytes to file '%s'.\n", s, pi->size, (char *)po->p);

		s = fclose(fp);
		if (s)
			ddq_warning("std/obj/copy : Something is wrong when closing file '%s'.\n", (char *)po->p);

	}
	else if (pi->type == obj_file_local && po->type == obj_file_local)
	{
		// TODO: 在不同系统上，拷贝文件可能需要不同的库函数：
		// 	linux : sendfile
		// 	windows : CopyFile ?
		// 	__APPLE__ : fcopyfile
		// 	__FreeBSD__ : copy_file_range ?
		// 注意尽量用内核态运行拷贝
		ddq_error("std/obj/copy : do not know how to copy Type#%d memory to Type#%d memory.\n", pi->type, po->type);
	}
	else
		ddq_error("std/obj/copy : do not know how to copy Type#%d memory to Type#%d memory.\n", pi->type, po->type);
}


#ifdef	enable_processor_cuda______//(编译BUG,先不编译)

static cudaStream_t	pick_cuda_stream()
{
	static cudaStream_t	stream;

	// TODO: 初始化和选择策略！

	return	stream;
}

static void		prepare_cuda(obj_mem po, int_t size)
{
	cudaStream_t stream = pick_cuda_stream();

	if (po->bufsize < size)
	{
		switch (po->type)
		{
			case	obj_mem_cuda_device :
				if (po->p)
					if (cudaFreeAsync(po->p, stream) != cudaSuccess)
						ddq_warning("std/obj/copy : Something is wrong when calling cudaFreeAsync().\n");
				if (cudaMallocAsync(&(po->p), size, stream) != cudaSuccess)
					ddq_error("std/obj/copy : Something is wrong when calling cudaMallocAsync().\n");
				break;

			case	obj_mem_cuda_host :
				if (po->p)
					if (po->type == obj_mem_default)
						free(po->p);
					else if (cudaFreeHost(po->p) != cudaSuccess)		// FIXME: 这里暂时没有考虑其他类型的内存
						ddq_warning("std/mem/copy : Something is wrong when calling cudaFreeHost().\n");
				if (cudaMallocHost(&(po->p), size) != cudaSuccess)		// NOTE: 这两个函数都没有进入流，可能会略微占用cpu时间
					ddq_error("std/mem/copy : Something is wrong when calling cudaMallocHost().\n");
				break;
		}
		po->bufsize = size;
	}
	po->size = size;
}

static void		copy_cuda(obj_mem po, obj_mem pi)
{
	cudaStream_t stream = pick_cuda_stream();

	if (pi->type == obj_mem_cuda_host && po->type == obj_mem_cuda_device)
	{
		if (cudaMemcpyAsync(po->p, pi->p, po->size, cudaMemcpyHostToDevice, stream) != cudaSuccess)
			ddq_error("std/obj/copy : Something is wrong when calling cudaMemcpyAsync().\n");
		// FIXME: 异步处理！
	}
	else if (pi->type == obj_mem_cuda_device && po->type == obj_mem_cuda_host)
	{
		if (cudaMemcpyAsync(po->p, pi->p, po->size, cudaMemcpyDeviceToHost, stream) != cudaSuccess)
			ddq_error("std/obj/copy : Something is wrong when calling cudaMemcpyAsync().\n");
		// FIXME: 异步处理！
	}
	else
		ddq_error("std/obj/copy : do not know how to copy Type#%d memory to Type#%d memory.\n", pi->type, po->type);
}

#endif


task_ret	copy(void **inputs, void **outputs, void **attribute)
{
	obj_mem	pi = (obj_mem)inputs[0];
	obj_mem	po = (obj_mem)outputs[0];

	if (pi->type == obj_mem_default && po->type == obj_mem_default)
	{
		prepare_default(po, pi->size);
		copy_default(po, pi);
	}

	else if (pi->type == obj_file_local || po->type == obj_file_local)
		copy_file(po, pi);

#ifdef	enable_processor_cuda
	else if (pi->type == obj_mem_cuda_host || pi->type == obj_mem_cuda_device || po->type == obj_mem_cuda_host || po->type == obj_mem_cuda_device)
	{
		if (po->type == obj_mem_default)
			prepare_default(po, pi->size);
		else if (po->type == obj_mem_cuda_device || po->type == obj_mem_cuda_host)
			prepare_cuda(po, pi->size);
		copy_cuda(po, pi);
	}
#endif

	else
		ddq_error("std/obj/copy : do not know how to copy Type#%d memory to Type#%d memory.\n", pi->type, po->type);

	return	task_ret_ok;
}


/*
 * 与类型转换有关的代码
 */



/*
 * 与文件IO有关的代码
 */

static void	set_file_name(obj_mem po, char *fn)
{
	if (po->p && po->type != obj_mem_default && po->type != obj_file_local)
		ddq_warning("obj/file_name : an unknown pointer is discarded.\n");

	po->size = strlen(fn) + 1;

	if (po->p && po->bufsize < po->size && (po->type == obj_mem_default || po->type == obj_file_local))
		free(po->p);

	if (!(po->p && po->bufsize >= po->size && (po->type == obj_mem_default || po->type == obj_file_local)))
	{
		po->bufsize = po->size;
		po->p = malloc(po->bufsize);
	}

	po->type = obj_file_local;
	strcpy(po->p, fn);
}

task_ret	file_import(void **inputs, void **outputs, void **attribute)
{
	set_file_name((obj_mem)outputs[0], inputs[0]);

	return	task_ret_done;
}

task_ret	file_export(void **inputs, void **outputs, void **attribute)
{
	obj_mem	p;

	p = new_mem();

	set_file_name(p, inputs[0]);
	copy(&inputs[1], (void **)&p, attribute);

	p->type = obj_mem_default;
	del_mem(p);

	return	task_ret_done;
}


