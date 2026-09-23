#ifndef NG_NATIVE_TEST_BFILE_H
#define NG_NATIVE_TEST_BFILE_H
#include <stdint.h>
enum {BFile_File=1,BFile_ReadOnly=1,BFile_WriteOnly=2,BFile_ReadWrite=3,BFile_EntryNotFound=-1};
int BFile_Remove(const uint16_t *path);
int BFile_Create(const uint16_t *path,int type,int *size);
int BFile_Open(const uint16_t *path,int mode);
int BFile_Close(int fd);
int BFile_Size(int fd);
int BFile_Seek(int fd,int offset);
int BFile_Write(int fd,const void *data,int size);
int BFile_Read(int fd,void *data,int size,int offset);
#endif
