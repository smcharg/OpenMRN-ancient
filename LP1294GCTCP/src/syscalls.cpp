/*
 * syscalls.cpp
 *
 *  Created on: Mar 14, 2016
 *      Author: sid
 *
 *  Minimal implementation of system calls to permit serial output via uart
 */

#include <new>
#include <stdio.h>

#include "UART.h"

extern UART
	serial_uart;


extern "C" int _close_r(struct _reent *reent,int fd)
{
	return(0);
}

extern "C" int _isatty_r(struct _reent *reent,int fd)
{
	return(1);
}

extern "C" _off_t _lseek_r(struct _reent *reent,int fd,_off_t offset,int whence)
{
	return(0);
}

extern "C" int _fstat_r(struct _reent *reent,int fd)
{
	return(0);
}

extern "C" int _open_r(const char *name,int mode,int flags)
{
	return(0);
}

extern "C" ssize_t _read_r(struct _reent *reent,int fd,void *buf,size_t count)
{
	return(0);
}

extern "C" ssize_t _write_r(struct _reent *reent,int fd,const void *buf,size_t count)
{
	char
		*ptr = (char *) buf;
	size_t
		len = 0;
	while (len < count)
	{
		serial_uart.Send(*ptr++);
		++len;
	}
	return(count);
}

extern "C" void _exit()
{
	return;
}

extern "C" int _kill_r(int pid)
{
	return(0);
}

extern "C" int _getpid_r()
{
	return(1);
}


