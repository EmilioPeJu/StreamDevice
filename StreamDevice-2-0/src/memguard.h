/***************************************************************
* memguard                                                     *
*                                                              *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* Include memguard.h and link memguard.o to get some memory    *
* guardiance. Allocated memory (using new) is tracked with     *
* source file and line of the new operator. Deleted memory is  *
* checked for duplicate deletion.                              *
* Call memguardReport() to see all allocated memory.           *
* Calling memguardReport() shows currently allocated memory.   *
*                                                              *
* DISCLAIMER: If this software breaks something or harms       *
* someone, it's your problem.                                  *
*                                                              *
***************************************************************/

#ifdef __cplusplus
#ifndef memguard_h
#define memguard_h
#include <stddef.h>
#define MEMGUARD
void* operator new(size_t, const char*, long);
void* operator new[](size_t, const char*, long);
#define new new(__FILE__,__LINE__)
int memguardLocation(const char*, long);
#define delete if (memguardLocation(__FILE__,__LINE__)) delete
void memguardReport();
#endif
#endif
