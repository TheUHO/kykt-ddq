
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"task_types.h"
#include	"error.h"


task_ret	op_shell(void **inputs, void **outputs, void **attribute)
{
	int	size, s;
	char	*buf, *p, *pb, *p0;
	int	status;
	int	ret;
	char	t;

	size = 0;
	status = 0;
	for (p = inputs[0]; *p; p++, size++)		// FIXME: 这里的size是高估了的，包含了所有被替代的标记
		switch (status)
		{
			case	0 :	// 不在标记内
				if (*p == '#')
					status = 1;
				break;
			case	1 :	// 刚进标记内
				if (*p == '-' || *p == '+')
				{
					status = 2;
					s = 0;
					t = *p;
				}
				else
					status = 0;
				break;
			case	2 :	// 输入标记
				if (*p == '#')
				{
					if (t=='-' && inputs[s])
						size += strlen(inputs[s]);
					else if (t=='+' && outputs[s])
						size += strlen(outputs[s]);
					else
						ddq_warning("op_shell() : parameters are not set properly.\n");
					status = 0;
				}
				else if ('0' <= *p && *p <= '9')
					s = s * 10 + (*p - '0');
				else
					status = 0;
		}
	if (((char*)(outputs[0]))[0])
		size += strlen(outputs[0]) + 4;
	buf = malloc(size+1);
	status = 0;
	for (p = inputs[0], pb = buf; *p; p++)
		switch (status)
		{
			case	0 :	// 不在标记内
				if (*p == '#')
				{
					p0 = p;
					status = 1;
				}
				else
					*pb++ = *p;
				break;
			case	1 :	// 刚进标记内
				if (*p == '-' || *p == '+')
				{
					status = 2;
					s = 0;
					t = *p;
				}
				else
				{
					for (; p0 != p; p0++, pb++)
						*pb = *p0;
					*pb++ = *p;
					status = 0;
				}
				break;
			case	2 :	// 输入标记
				if (*p == '#')
				{
					if (t=='-' && inputs[s])
						p0 = inputs[s];
					else if (t=='+' && outputs[s])
						p0 = outputs[s];
					else
						p0 = "";
					strncpy(pb, p0, strlen(p0));
					pb += strlen(p0);
					status = 0;
				}
				else if ('0' <= *p && *p <= '9')
					s = s * 10 + (*p - '0');
				else
				{
					for (; p0 != p; p0++, pb++)
						*pb = *p0;
					*pb++ = *p;
					status = 0;
				}
		}
	if (((char*)(outputs[0]))[0])
	{
		strncpy(pb, " >> ", 4);
		pb += 4;
		strncpy(pb, outputs[0], strlen(outputs[0]));
		pb += strlen(outputs[0]);
	}
	*pb = '\0';

	ddq_log("op_shell() : Start running command : '%s'\n", buf);
	ret = system(buf);
	ddq_log("op_shell() : Finish running command and the return value is : %d\n", ret);

	free(buf);

	return	task_ret_ok;
}

