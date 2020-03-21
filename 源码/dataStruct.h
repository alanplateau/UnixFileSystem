#pragma once
#include "define.h"
class SuperBlock {
public:
	unsigned int diskSize;
	unsigned int freeBlock[FREE_BLOCK_NUM];
	int freeBlockIndex;
	unsigned int freeTotalBlock;
	unsigned int freeInode[FREE_INODE_NUM];
	int freeInodeIndex;
	int freeTotalInode;
	long int lastLoginTime;
	char umask[UMASK_SIZE];
};

class Finode {
public:
	long int fileSize;
	int fileLink;
	int addr[6];
	char owner[OWNER_MAX_NAME];
	char group[GROUP_MAX_NAME];
	int mode;
	long int modifiedTime;
	char blank[48];
};

class Inode {
public:
	Finode finode;
	unsigned short int parentInodeID;
	unsigned short int inodeID;

};

class DirectEntry {
public:
	unsigned short int inodeID;
	char directEntryName[DIRECT_MAX_NAME];
};

class Direct {
public:
	int directEntryNum;
	DirectEntry directEntry[ENTRY_NUM_IN_BLOCK];
};

class FileStruct {
public:
	unsigned short int inodeID;
	int filePointer;
};

class User {
public:
	char username[OWNER_MAX_NAME];
	char password[MAX_PASSWORD];
	char groupname[GROUP_MAX_NAME];
};




