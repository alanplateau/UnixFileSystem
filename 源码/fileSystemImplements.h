#pragma once
#include "dataStruct.h"
#include <map>
#include <fstream>
#include<iostream>
#include "error.h"
#include <time.h>
#include<conio.h>
#include <stack>
#include <queue>
#include<sstream>
#include<Windows.h>
#include<iomanip>
#pragma warning(disable:4996)

using namespace std;

SuperBlock* super;

//map<unsigned int, Inode> vistedInode;

fstream virtualDisk;

FileStruct* opened[];

Inode currentInode;

Inode* root;

User* currentUser;

int userIndex;

Inode userInfoInode;

User* users;

int usernum;


//少一个当前文件项

//Direct* currentDirect;

bool logout = false;

inline int getInodePos(int inodeID) {
	return SUPERSIZE + inodeID * sizeof(Finode);
}

inline int getBlockPos(int blockNumber) {
	return BLOCKSIZE * blockNumber;
}

int synchronizationInode(Inode* inode) {
	int inodeID = inode->inodeID;
	//cout << "write inodeID" << inodeID << endl;
	//vistedInode[inodeID] = *inode;
	Finode finode = inode->finode;
	//cout << "finode.filesize---" << finode.fileSize << endl;
	//cout << "getInodePos" << getInodePos(inodeID) << endl;
	virtualDisk.seekg(getInodePos(inodeID), ios::beg);
	virtualDisk.write((const char*)&finode, sizeof(Finode));
	//cout << "syn filesize" << inode->finode.fileSize << endl;
	virtualDisk.flush();
	return SYNC_OK;

}



int synchronizationSuperBlock() {
	if (!virtualDisk.is_open()) {
		cerr << "unopened virtualDisk" << endl;
		return SYNC_SUPER_FAILED;
	}
	virtualDisk.seekg(0, ios::beg);
	virtualDisk.write((const char*)super, sizeof(SuperBlock));
	virtualDisk.flush();
	return SYNC_OK;
}



int getFreeBlockID() {
	int freeBlockID = super->freeBlock[super->freeBlockIndex];
	super->freeTotalBlock--;
	if (super->freeBlockIndex == 0) {
		if (freeBlockID == 0) {
			return USE_OUT_OF_BLOCK;
		}
		virtualDisk.seekg(getBlockPos(freeBlockID), ios::beg);
		virtualDisk.read((char*)&super->freeBlockIndex, sizeof(int));
		for (int i = 0;i <= super->freeBlockIndex;i++) {
			virtualDisk.read((char*)&super->freeBlock[i], sizeof(unsigned int));
		}
		synchronizationSuperBlock();
	}
	else {
		super->freeBlockIndex--;
		synchronizationSuperBlock();
	}
	return freeBlockID;
}

void initNode(Inode* inode) {
	inode->finode.fileLink = 1;
	strcpy(inode->finode.owner, currentUser->username);
	strcpy(inode->finode.group, currentUser->groupname);
	time_t timer;
	time(&timer);
	inode->finode.modifiedTime = timer;
}

int getFreeInodeID() {
	int freeInodeID = super->freeInode[super->freeInodeIndex];
	super->freeTotalInode--;
	if (super->freeInodeIndex == 0) {
		if (super->freeTotalInode == 0) {
			return USE_OUT_OF_NODE;
		}
		Finode tempFinode;
		int i, j;
		for (i = 1,j=0;j < FREE_INODE_NUM&&i<=TOTAL_INODE;i++) {
			virtualDisk.seekg(getInodePos(i), ios::beg);
			virtualDisk.read((char*)&tempFinode, sizeof(Finode));
			if (tempFinode.fileLink == 0) {
				super->freeInode[j] = i;
				j++;
			}
		}
		super->freeInodeIndex = j-1;
		synchronizationSuperBlock();
	}
	else
	{
		super->freeInodeIndex--;
		synchronizationSuperBlock();
	}
	return freeInodeID;
}

void blockFree(int freeBlockID) {
	if (super->freeBlockIndex == FREE_BLOCK_NUM - 1) {
		virtualDisk.seekg(getBlockPos(freeBlockID), ios::beg);
		virtualDisk.write((const char*)&super->freeBlockIndex, sizeof(int));
		for (int i = 0;i <= super->freeBlockIndex;i++) {
			virtualDisk.write((const char*)&super->freeBlock[i], sizeof(unsigned int));
		}
		super->freeBlockIndex = 0;
		super->freeBlock[0] = freeBlockID;
		super->freeTotalBlock++;
		synchronizationSuperBlock();
	}
	else {
		super->freeBlockIndex++;
		super->freeBlock[super->freeBlockIndex] = freeBlockID;
		//cout << "free's blockID" << freeBlockID << endl;
		super->freeTotalBlock++;
		synchronizationSuperBlock();
	}

}



Inode getInode(int inodeID,int parentInodeID=-1) {
	/*if (vistedInode.count(inodeID) > 0) {
		map<unsigned int, Inode>::iterator it = vistedInode.find(inodeID);
		                                                                                                                                                                                             
		return it->second;
	}*/
	virtualDisk.seekg(getInodePos(inodeID), ios::beg);
	Inode newInode;
	newInode.inodeID = inodeID;
	if (parentInodeID == -1) {
		newInode.parentInodeID = currentInode.inodeID;
	}
	else
	{
		newInode.parentInodeID = parentInodeID;
	}
	virtualDisk.read((char*)&(newInode.finode), sizeof(Finode));
	//vistedInode[inodeID] = newInode;
	return newInode;
}


int getOwnerRight(int mode) {
	mode = mode % 1000;
	mode = mode / 100;
	return mode;
}

int getGroupRight(int mode) {
	mode = mode % 1000;
	mode = mode % 100;
	mode = mode / 10;
	return mode;
}

bool hasWrite(int singleMode) {
	if (singleMode == 2 || singleMode == 3 || singleMode == 6 || singleMode == 7) {
		return true;
	}
	return false;
}

bool hasExecute(int singleMode) {
	if (singleMode == 1 || singleMode == 3 || singleMode == 5 || singleMode == 7) {
		return true;
	}
	return false;
}


int getOthersRight(int mode) {
	mode = mode % 10;
	return mode;
}

//第一个字节的原因吗
void writeUsers() {
	//root->finode.addr[0] = getFreeBlockID();
	Direct* newDir = new Direct();
	virtualDisk.seekg(getBlockPos(root->finode.addr[0]), ios::beg);
	virtualDisk.read((char*)newDir, sizeof(Direct));
	int num = newDir->directEntryNum;
	strcpy(newDir->directEntry[num].directEntryName, "user");
	newDir->directEntry[num].inodeID = getFreeInodeID();
	newDir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(root->finode.addr[0]), ios::beg);
	virtualDisk.write((const char*)newDir, sizeof(Direct));
	//root->finode.fileSize = 512;
	synchronizationInode(root);
	//syn

	Inode userInfoInode = getInode(newDir->directEntry[num].inodeID);
	//initNode(&userInfoInode);
	userInfoInode.finode.fileLink = 1;
	//要乘2
	userInfoInode.finode.mode = 644;
	time_t timer;
	time(&timer);
	userInfoInode.finode.modifiedTime = timer;
	strcpy(userInfoInode.finode.group, "root");
	strcpy(userInfoInode.finode.owner, "root");
	userInfoInode.finode.fileSize = 2*sizeof(User);
	userInfoInode.parentInodeID = root->inodeID;
	userInfoInode.finode.addr[0] = getFreeBlockID();
	User rootuser;
	strcpy(rootuser.username,"root");
	strcpy(rootuser.groupname, "root");
	strcpy(rootuser.password, "1234");

	User normalUser;
	strcpy(normalUser.username, "hello");
	strcpy(normalUser.groupname, "hello");
	strcpy(normalUser.password, "1234");
	cout << "userInoInode.finode.addr[0]" << userInfoInode.finode.addr[0] << endl;
	virtualDisk.seekg(getBlockPos(userInfoInode.finode.addr[0]), ios::beg);
	virtualDisk.write((const char*)&rootuser, sizeof(User));
	virtualDisk.write((const char*)&normalUser, sizeof(User));
	synchronizationInode(&userInfoInode);


	//root->finode.fileSize = 512;
	//synchronizationInode(root);


}



int modeCalculator(int mode) {
	int hundred = mode / 100;
	int ten = (mode % 100) / 10;
	int one = mode % 10;
	hundred -= (int)super->umask[1];
	ten -= (int)super->umask[2];
	one-= (int)super->umask[3];
	return one + ten * 10 + hundred * 100;
}

void getMode(int mode)
{
	int type = mode / 1000;
	int auth = mode % 1000;
	if (type == 1)
		cout << "d";
	else
		cout << "-";
	int div = 100;
	for (int i = 0;i < 3;i++)
	{
		int num = auth / div;
		auth = auth % div;
		int a[3] = { 0 };
		int time = 2;
		while (num != 0)
		{
			a[time--] = num % 2;
			num = num / 2;
		}
		for (int i = 0;i < 3;i++)
		{
			if (a[i] == 1)
			{
				if (i == 2)
					cout << "x";
				else if (i == 1)
					cout << "w";
				else if (i == 0)
					cout << "r";
			}
			else
				cout << "-";
		}
		div /= 10;
	}
}

int numberToPos(int writeNumber, int addr[3]) {
	if (writeNumber <= 4) {
		addr[0] = writeNumber;
		return 1;
	}
	else if (writeNumber <= 132) {
		addr[0] = 5;
		addr[1] = writeNumber - 4;
		return 2;
	}
	else
	{
		addr[0] = 6;
		addr[1] = (writeNumber - 133) / 128 + 1;
		addr[2] = ((writeNumber - 132) % 128) == 0 ? 128 : ((writeNumber - 132) % 128);
		return 3;

	}
}

void getTime(long int timeStamp)
{
	time_t timer;
	timer = timeStamp;
	struct tm* p;
	p = gmtime(&timer);
	char s[80];
	strftime(s, 80, "%Y-%m-%d %H:%M:%S", p);
	printf("%s", s);
}

int findFileInDir(Inode givenInode, char* filename,int &targetFreeBlockID) {
	int sumAddrNumber = givenInode.finode.fileSize / 512;
	int freeAddrNumber = -1;
	int flag = false;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(givenInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			if (tempDir->directEntryNum < 31 && flag == false) {
				freeAddrNumber = i + 1;
				targetFreeBlockID = givenInode.finode.addr[i];
				flag = true;
			}
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					return tempDir->directEntry[j].inodeID;
				}
			}
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = givenInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			Direct* tempDir = new Direct();
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			if (tempDir->directEntryNum < 31 && flag == false) {
				freeAddrNumber = i + 5;
				targetFreeBlockID = currentBlock;
				flag = true;
			}
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					return tempDir->directEntry[j].inodeID;
				}
			}
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = givenInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax = 128;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				Direct* tempDir = new Direct();
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				if (tempDir->directEntryNum < 31 && flag == false) {
					freeAddrNumber = 128 + 4 + 128 * x + i + 1;
					targetFreeBlockID = currentBlock;
					flag = true;
				}
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
						return tempDir->directEntry[j].inodeID;
					}
				}
			}
		}

	}
	if (freeAddrNumber == -1)targetFreeBlockID = -1;
	return -1;
}

void insertDirectEntry(int targetFreeBlockID, char* filename) {
	Direct* dir = new Direct();
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.read((char*)dir, sizeof(Direct));


	int newInodeID = getFreeInodeID();
	Inode* newInode = new Inode();
	newInode->inodeID = newInodeID;
	newInode->parentInodeID = currentInode.inodeID;
	newInode->finode.fileLink = 1;
	newInode->finode.fileSize = 512;
	newInode->finode.addr[0] = getFreeBlockID();
	Direct* sonDir = new Direct();
	sonDir->directEntryNum = 2;
	DirectEntry pointDirectory;
	strcpy(pointDirectory.directEntryName, ".");
	pointDirectory.inodeID = newInodeID;
	DirectEntry pointpointEntry;
	strcpy(pointpointEntry.directEntryName, "..");
	pointpointEntry.inodeID = currentInode.inodeID;
	sonDir->directEntry[0] = pointDirectory;
	sonDir->directEntry[1] = pointpointEntry;
	virtualDisk.seekg(getBlockPos(newInode->finode.addr[0]), ios::beg);
	virtualDisk.write((const char*)sonDir, sizeof(Direct));
	time_t timer;
	time(&timer);
	newInode->finode.modifiedTime = timer;
	strcpy(newInode->finode.owner, currentUser->username);
	strcpy(newInode->finode.group, currentUser->groupname);
	newInode->finode.mode = 1000 + modeCalculator(766);
	synchronizationInode(newInode);

	DirectEntry newEntry;
	newEntry.inodeID = newInode->inodeID;
	strcpy(newEntry.directEntryName, filename);

	dir->directEntry[dir->directEntryNum] = newEntry;
	dir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.write((const char*)dir, sizeof(Direct));

}


int mkdir(char *filename) {
	int sumAddrNumber = currentInode.finode.fileSize / 512;
	int freeAddrNumber = -1;
	int flag = false;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]),ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			if (tempDir->directEntryNum < 31 && flag == false) {
				freeAddrNumber = i+1;
				flag = true;
			}
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					cout << "Can't create Directory" << filename << ":file exists" << endl;
					return FILE_EXIST;
				}
			}
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = currentInode.finode.addr[4];
		for (int i = 0;i < addrNumber-4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			Direct *tempDir = new Direct();
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			if (tempDir->directEntryNum < 31 && flag == false) {
				freeAddrNumber = i+5;
				flag = true;
			}
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					cout << "Can't create Directory" << filename << ":file exists" << endl;
					return FILE_EXIST;
				}
			}
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = currentInode.finode.addr[5];
		for (int x = 0;x <=(addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax=128;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				Direct* tempDir = new Direct();
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				if (tempDir->directEntryNum < 31 && flag == false) {
					freeAddrNumber = 128+4+128*x+i+1;
					flag = true;
				}
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
						cout << "Can't create Directory" << filename << ":file exists" << endl;
						return FILE_EXIST;
					}
				}
			}
		}

	}
	
    int targetFreeBlockID;
	if (freeAddrNumber == -1) {
		if (sumAddrNumber < 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[addrNumber] = targetFreeBlockID;
		}
		else if (sumAddrNumber == 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[4] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[4]), ios::beg);
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber < 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int addr4 = currentInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (addrNumber - 4 ), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber == 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[5] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]), ios::beg);
			int secondBlock = getFreeBlockID();
			virtualDisk.write((const char*)&secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int secondPos = (addrNumber - 133) / 128+1;
			int firstPos = ((addrNumber - 132) % 128)==0?128: ((addrNumber - 132) % 128);
			if (firstPos == 128) {
				int newSecondBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * secondPos, ios::beg);
				virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				secondPos++;
				firstPos = 0;
				
			}
			int secondBlock=-1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (secondPos-1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (firstPos), sizeof(int));
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));


		}
		Direct* newDir = new Direct();
		newDir->directEntryNum = 0;
		virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
		virtualDisk.write((const char*)newDir, sizeof(Direct));
		currentInode.finode.fileSize += 512;
		synchronizationInode(&currentInode);
	}
	else {
		if (freeAddrNumber <= 4) {
			targetFreeBlockID = currentInode.finode.addr[freeAddrNumber - 1];
		}
		else if (freeAddrNumber <= 132) {
			int addr4 = currentInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (freeAddrNumber - 5), ios::beg);
			virtualDisk.read((char*)&targetFreeBlockID, sizeof(int));
		}
		else {
			int addr[3];
			numberToPos(freeAddrNumber, addr);
			int secondBlock=-1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5])+sizeof(int)*(addr[1]-1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (addr[2] - 1), ios::beg);
			virtualDisk.read((char*)&targetFreeBlockID, sizeof(int));
		}

	}
	insertDirectEntry(targetFreeBlockID, filename);
	return 1;
}

void _rmdir(Inode rmInode) {
	int sumAddrNumber = rmInode.finode.fileSize / 512;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(rmInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp(tempDir->directEntry[j].directEntryName, ".") == 0 || strcmp(tempDir->directEntry[j].directEntryName, "..") == 0) {
					continue;
				}
				Inode tempInode = getInode(tempDir->directEntry[j].inodeID);
				if (tempInode.finode.mode / 1000 == 1) {
					_rmdir(tempInode);
				}
				else {
					if (tempInode.finode.fileSize == 0) {}
					else
					{
						int tempaddrNumber = (tempInode.finode.fileSize - 1) / 512 + 1;
						if (tempaddrNumber > 0) {
							int temptempAddrNumber = (tempaddrNumber > 4) ? 4 : tempaddrNumber;
							for (int k = 0;k < temptempAddrNumber;k++) {
								blockFree(tempInode.finode.addr[k]);
							}
						}
						if (tempaddrNumber > 4) {
							int temptempAddrNumber = (tempaddrNumber > 132) ? 132 : tempaddrNumber;
							int addr4 = tempInode.finode.addr[4];
							for (int x = 0;x < temptempAddrNumber - 4;x++) {
								virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * x, ios::beg);
								int currentBlock;
								virtualDisk.read((char*)&currentBlock, sizeof(int));
								blockFree(currentBlock);
							}
							blockFree(addr4);
						}
						if (tempaddrNumber > 132) {}
					}
				}
				tempInode.finode.fileLink = 0;
				tempInode.finode.fileSize == 0;
				synchronizationInode(&tempInode);
				super->freeTotalInode++;
				synchronizationSuperBlock();
			}
			blockFree(rmInode.finode.addr[i]);
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = rmInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			Direct* tempDir = new Direct();
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				Inode tempInode = getInode(tempDir->directEntry[j].inodeID);
				if (tempInode.finode.mode / 1000 == 1) {
					_rmdir(tempInode);
				}
				if (tempInode.finode.fileSize == 0) {}
				else
				{
					int tempaddrNumber = (tempInode.finode.fileSize - 1) / 512 + 1;
					if (tempaddrNumber > 0) {
						int temptempAddrNumber = (tempaddrNumber > 4) ? 4 : tempaddrNumber;
						for (int k = 0;k < temptempAddrNumber;k++) {
							blockFree(tempInode.finode.addr[k]);
						}
					}
					if (tempaddrNumber > 4) {
						int temptempAddrNumber = (tempaddrNumber > 132) ? 132 : tempaddrNumber;
						int addr42 = rmInode.finode.addr[4];
						for (int x = 0;x < temptempAddrNumber - 4;x++) {
							virtualDisk.seekg(getBlockPos(addr42) + sizeof(int) * x, ios::beg);
							int currentBlock;
							virtualDisk.read((char*)&currentBlock, sizeof(int));
							blockFree(currentBlock);
						}
						blockFree(addr42);
					}
					if (tempaddrNumber > 132) {}
				}
				tempInode.finode.fileLink = 0;
				tempInode.finode.fileSize == 0;
				synchronizationInode(&tempInode);
			}
			blockFree(currentBlock);
		}
		blockFree(addr4);

	}
	if(sumAddrNumber>132){}
	return;
}

int rmdir(char * filename) {
	if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
		cout << "rmdir:failed to remove " << filename << ":Invalid argument";
		return -1;
	}
	int sumAddrNumber = currentInode.finode.fileSize / 512;
	bool flag = false;
	Inode rmInode;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					rmInode = getInode(tempDir->directEntry[j].inodeID);
					for (int k = j;k < tempDir->directEntryNum;k++)
						tempDir->directEntry[k] = tempDir->directEntry[k + 1];
					tempDir->directEntryNum--;
					virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
					virtualDisk.write((const char*)tempDir, sizeof(Direct));
					flag = true;
					break;
				}
			}
			if (flag)break;
		}
	}
	if (sumAddrNumber > 4 &&flag==false) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = currentInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					rmInode = getInode(tempDir->directEntry[j].inodeID);
					for (int k = j;k < tempDir->directEntryNum;k++)
						tempDir->directEntry[k] = tempDir->directEntry[k + 1];
					tempDir->directEntryNum--;
					virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
					virtualDisk.write((const char*)tempDir, sizeof(Direct));
					flag = true;
					break;
				}
			}
			if (flag)break;
		}
	}
	if (sumAddrNumber > 132 &&flag==false) {
		int addrNumber = sumAddrNumber;
		int addr5 = currentInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				Direct* tempDir = new Direct();
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
						rmInode = getInode(tempDir->directEntry[j].inodeID);
						for (int k = j;k < tempDir->directEntryNum;k++)
							tempDir->directEntry[k] = tempDir->directEntry[k + 1];
						tempDir->directEntryNum--;
						virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
						virtualDisk.write((const char*)tempDir, sizeof(Direct));
						flag = true;
						break;
					}
				}
				if (flag)break;
			}
		}

	}
	if (flag == false) {
		cout << "failed to remove" << filename << ":no such directory";
		return NO_SAME_DIR;
	}
	else
	{
		_rmdir(rmInode);
		rmInode.finode.fileLink = 0;
		rmInode.finode.fileSize = 0;
		synchronizationInode(&rmInode);
		super->freeTotalInode++;
		synchronizationSuperBlock();
	}
	return 1;

}


int cd(char* path) {
	char* dirNames[20];
	Inode tempCurrentInode;
	if (path[0] == '/') {
		tempCurrentInode = *root;
	}
	else {
		tempCurrentInode = currentInode;
	}
	//cout << "cd start" << endl;
	dirNames[0] = strtok(path, "/");
	int i = 0;
	while (dirNames[i] != NULL)
	{
		i++;
		dirNames[i] = strtok(NULL, "/");
	}
	dirNames[i] = "\0";
	/*for (int j = 0; j < i; j++)
	{
		cout << dirNames[j] << endl;
	}*/
	int targetInodeID;
	for (int j = 0;j < i;j++) {
		int targetFreeBlockID;
		targetInodeID = findFileInDir(tempCurrentInode, dirNames[j], targetFreeBlockID);
		if (targetInodeID == -1) {
			cout << "can not find directory" << dirNames[j] << endl;
			return -1;
		}
		tempCurrentInode = getInode(targetInodeID, tempCurrentInode.inodeID);
		if (tempCurrentInode.finode.mode / 1000 != 1) {
			cout << dirNames[j] << "is not a directory" << endl;
			return -1;
		}
		if (strcmp(currentUser->username, "root") != 0) {
			if (strcmp(currentUser->username, tempCurrentInode.finode.owner) == 0) {
				int ownerRight = getOwnerRight(tempCurrentInode.finode.mode);
				if (!hasExecute(ownerRight)) {
					cout << "Permission Denied" << endl;
					return -1;
				}
			}
			else if (strcmp(currentUser->groupname, tempCurrentInode.finode.group) == 0) {
				int groupRight = getGroupRight(tempCurrentInode.finode.mode);
				if (!hasExecute(groupRight)) {
					cout << "Permission Denied" << endl;
					return -1;
				}
			}
			else {
				int othersRight = getOthersRight(tempCurrentInode.finode.mode);
				if (!hasExecute(othersRight)) {
					cout << "Permission Denied" << endl;
					return -1;
				}
			}
		}

	}
	currentInode = tempCurrentInode;

	return 1;
}

int ls() {
	int sumAddrNumber = currentInode.finode.fileSize / 512;
	int outputControl = 0;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if ((strcmp(tempDir->directEntry[j].directEntryName,".") == 0)||strcmp(tempDir->directEntry[j].directEntryName,"..") == 0){
					continue;
				}
				cout << tempDir->directEntry[j].directEntryName << " ";
				outputControl++;
				if (outputControl == 11) {
					outputControl = 0;
					cout << endl;
				}
			}
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = currentInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			Direct* tempDir = new Direct();
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				cout << tempDir->directEntry->directEntryName << " ";
				outputControl++;
				if (outputControl == 11) {
					outputControl = 0;
					cout << endl;
				}
			}
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = currentInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				Direct* tempDir = new Direct();
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					cout << tempDir->directEntry->directEntryName << " ";
					outputControl++;
					if (outputControl == 11) {
						outputControl = 0;
						cout << endl;
					}
				}
			}
		}

	}
	return 1;
}

int ll() {
	int sumAddrNumber = currentInode.finode.fileSize / 512;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				Inode tempInode = getInode(tempDir->directEntry[j].inodeID);
				getMode(tempInode.finode.mode);
				cout << " ";
				cout << setw(6);
				//cout.write(tempInode.finode.owner, strlen(tempInode.finode.owner));
				cout << tempInode.finode.owner;
				cout << " ";
				cout << setw(6);
				//cout.write(tempInode.finode.group, strlen(tempInode.finode.group));
				cout << tempInode.finode.group;
				cout << " ";
				cout << setw(5);
				cout << tempInode.finode.fileSize << " ";
				getTime(tempInode.finode.modifiedTime);
				cout << " ";
				cout << tempDir->directEntry[j].directEntryName << endl;
			}
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = currentInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			Direct* tempDir = new Direct();
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				Inode tempInode = getInode(tempDir->directEntry[j].inodeID);
				getMode(tempInode.finode.mode);
				cout << " ";
				cout << setw(6);
				//cout.write(tempInode.finode.owner, strlen(tempInode.finode.owner));
				cout << tempInode.finode.owner;
				cout << " ";
				cout << setw(6);
				//cout.write(tempInode.finode.group, strlen(tempInode.finode.group));
				cout << tempInode.finode.group;
				cout << " ";
				cout << setw(5);
				cout << tempInode.finode.fileSize << " ";
				getTime(tempInode.finode.modifiedTime);
				cout << " ";
				cout << tempDir->directEntry[j].directEntryName << endl;
			}
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = currentInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				Direct* tempDir = new Direct();
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					Inode tempInode = getInode(tempDir->directEntry[j].inodeID);
					getMode(tempInode.finode.mode);
					cout << " ";
					cout << setw(6);
					//cout.write(tempInode.finode.owner, strlen(tempInode.finode.owner));
					cout << tempInode.finode.owner;
					cout << " ";
					cout << setw(6);
					//cout.write(tempInode.finode.group, strlen(tempInode.finode.group));
					cout << tempInode.finode.group;
					cout << " ";
					cout << setw(5);
					cout << tempInode.finode.fileSize << " ";
					getTime(tempInode.finode.modifiedTime);
					cout << " ";
					cout << tempDir->directEntry[j].directEntryName << endl;
				}
			}
		}

	}
	return 1;
}

int password() {
	char ch;
	char oldPassword[MAX_PASSWORD] = { 0 };
	char newPassword[MAX_PASSWORD] = { 0 };
	char rePassword[MAX_PASSWORD] = { 0 };
	cout << "please input the old password:";
	for (int i = 0;i < MAX_PASSWORD;i++)
	{
		ch = getch();
		if (ch == 13)
		{
			break;
		}
		oldPassword[i] = ch;
		cout << "*";
	}
	cout << endl;
	if (strcmp(oldPassword, currentUser->password) == 0)
	{
		cout << "new password:";
		for (int i = 0;i < MAX_PASSWORD;i++)
		{
			ch = getch();
			if (ch == 13)
			{
				break;
			}
			newPassword[i] = ch;
			cout << "*";
		}
		cout << endl;
		cout << "repeat the new password:";
		for (int i = 0;i < MAX_PASSWORD;i++)
		{
			ch = getch();
			if (ch == 13)
			{
				break;
			}
			rePassword[i] = ch;
			cout << "*";
		}
		cout << endl;
		if (strcmp(newPassword, rePassword) == 0)
		{
			strcpy(currentUser->password, newPassword);
			virtualDisk.seekg(getBlockPos(userInfoInode.finode.addr[0]) + sizeof(User) * userIndex, ios::beg);
			virtualDisk.write((const char*)currentUser, sizeof(User));
		}
		else
		{
			cout << "the password is not matched!" << endl;
		}
	}
	else
		cout << "the old password is not right!" << endl;
	return 1;
}

int pwd() {
	//cout << "pwd start" << endl;
	stack<string> pwd;
	Inode inode = currentInode;
	Inode parent ;
	Inode sonInode;
	int parentID;
	while (inode.inodeID != root->inodeID)
	{
		int targetFreeBlockID;
		int soninodeid = inode.inodeID;
		sonInode = getInode(soninodeid);
		parentID = findFileInDir(sonInode, "..", targetFreeBlockID);
		parent = getInode(parentID);
		//cout << "current" << inode.inodeID << endl;
		//cout << "parent" << parent.inodeID << endl;

		int sumAddrNumber = parent.finode.fileSize / 512;
		int flag = false;
		
		if (sumAddrNumber > 0) {
			int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
			for (int i = 0;i < addrNumber;i++) {
				Direct* tempDir = new Direct();
				virtualDisk.seekg(getBlockPos(parent.finode.addr[i]), ios::beg);
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (tempDir->directEntry[j].inodeID==soninodeid) {
						string partPwd(tempDir->directEntry[j].directEntryName);
						pwd.push(partPwd);
						inode = parent;
						flag = true;
						break;
					}
				}
				if (flag)break;
			}
		}
		if (sumAddrNumber > 4 && flag == false) {
			int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
			int addr4 = parent.finode.addr[4];
			for (int i = 0;i < addrNumber - 4;i++) {
				virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
				int currentBlock;
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				Direct* tempDir = new Direct();
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (tempDir->directEntry[j].inodeID == soninodeid) {
						string partPwd(tempDir->directEntry[j].directEntryName);
						pwd.push(partPwd);
						inode = parent;
						flag = true;
						break;
					}
				}
				if (flag)break;
			}
		}
		if (sumAddrNumber > 132 && flag == false) {
			int addrNumber = sumAddrNumber;
			int addr5 = parent.finode.addr[5];
			for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
				virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
				int secondBlock;
				virtualDisk.read((char*)&secondBlock, sizeof(int));
				int iMax;
				if (x == (addrNumber - 133) / 128) {
					int imax1 = (addrNumber - 132) % 128;
					iMax = (imax1 == 0) ? 128 : imax1;
				}
				for (int i = 0;i < iMax;i++) {
					int currentBlock;
					virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
					virtualDisk.read((char*)&currentBlock, sizeof(int));
					Direct* tempDir = new Direct();
					virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
					virtualDisk.read((char*)tempDir, sizeof(Direct));
					for (int j = 0;j < tempDir->directEntryNum;j++) {
						if (tempDir->directEntry[j].inodeID == soninodeid) {
							string partPwd(tempDir->directEntry[j].directEntryName);
							pwd.push(partPwd);
							inode = parent;
							flag = true;
							break;
						}
					}
					if (flag)break;
				}
			}

		}
	}
	if (pwd.empty())
		cout << "/";
	else
	{
		while (!pwd.empty())
		{
			cout << "/";
			cout << pwd.top();
			pwd.pop();
		}
	}
	//cout << endl;
	return 1;
}

int superInfo() {
	cout << "系统大小" << super->diskSize << endl;
	cout << "剩余空闲节点总个数" << super->freeTotalInode << endl;
	cout << "剩余空闲盘块总个数" << super->freeTotalBlock << endl;
	cout << "空闲盘块组当前下标" << super->freeBlockIndex << endl;
	cout << "剩余空闲盘块列表" << endl;
	int change = 0;
	for (int i = 0;i <= super->freeBlockIndex;i++) {
		cout << super->freeBlock[i] << " ";
		change++;
		if (change == 11) {
			cout << endl;
			change = 0;
		}
	}
	cout << endl;
	//cout << "下一个空闲盘块" << super->freeBlock[super->freeBlockIndex] << endl;
	cout << "空闲节点组当前下标" << super->freeInodeIndex << endl;
	cout<< "剩余空闲盘块列表" << endl;
	for (int i =0;i <= super->freeInodeIndex;i++) {
		cout << super->freeInode[i] << " ";
	}
	cout << endl;
	//cout << "下一个空闲节点" << super->freeInode[super->freeInodeIndex] << endl;
	return 1;
}



int putContenToBlock(queue<string>tempString, int writeNumber, Inode* inode) {
	int addr[3];
	int result = numberToPos(writeNumber, addr);
	int newBlock = getFreeBlockID();
	inode->finode.fileSize += 512;
	if (result == 1) {
		inode->finode.addr[writeNumber-1] = newBlock;
	}
	else if (result == 2) {
		if (writeNumber == 5) {
			inode->finode.addr[4] = getFreeBlockID();
		}
		virtualDisk.seekg(getBlockPos(inode->finode.addr[4])+sizeof(int)*(addr[1]-1), ios::beg);
		virtualDisk.write((const char*)&newBlock, sizeof(int));
	}
	else if (result == 3) {
		if (writeNumber == 133) {
			inode->finode.addr[5] = getFreeBlockID();
		}
		if (addr[2] == 1) {
			int secondAddr = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(inode->finode.addr[5]) + sizeof(int) * (addr[1] - 1), ios::beg);
			virtualDisk.write((const char*)&secondAddr, sizeof(int));
		}
		int secondAddrTemp;
		virtualDisk.seekg(getBlockPos(inode->finode.addr[5]) + sizeof(int) * (addr[1] - 1), ios::beg);
		virtualDisk.read((char*)&secondAddrTemp, sizeof(int));
		virtualDisk.seekg(getBlockPos(secondAddrTemp) + sizeof(int) * (addr[2] - 1), ios::beg);
		virtualDisk.write((const char*)&newBlock, sizeof(int));
	}
	virtualDisk.seekg(getBlockPos(newBlock), ios::beg);
	while (!tempString.empty()) {
		string str1 = tempString.front();
		const char* str2 = str1.data();
		virtualDisk.write(str2, str1.length());
		tempString.pop();
	}
	return 1;
}

int putContentToNewFile(int targetFreeBlockID,char *filename) {
	Direct* dir = new Direct();
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.read((char*)dir, sizeof(Direct));


	int newInodeID = getFreeInodeID();
	Inode* newInode = new Inode();
	newInode->inodeID = newInodeID;
	initNode(newInode);
	newInode->parentInodeID = currentInode.inodeID;
	newInode->finode.mode = modeCalculator(666);
	//newInode->finode.fileLink = 1;
	newInode->finode.fileSize = 0;
	//newInode->finode.addr[0] = getFreeBlockID();
	char* temp = new char[512];
	int sum = 0;
	queue<string> inputStack;
	//int writeBlockID = newInode->finode.addr[0];
	int writeNumber = 1;
	while (cin.getline(temp,512))
	{
		string tempString(temp);
		tempString.push_back('\n');
		sum += tempString.length();
		if (sum <= 512) {
			//string tempString(temp);
			inputStack.push(tempString);
		}
		else
		{
			inputStack.push(tempString.substr(0, tempString.length()-(sum - 512)));
			putContenToBlock(inputStack, writeNumber, newInode);
			while (!inputStack.empty())
				inputStack.pop();
			sum = 0 + sum-512;
			writeNumber++;
			inputStack.push(tempString.substr(tempString.length() -sum,sum));
		}
	}
	putContenToBlock(inputStack, writeNumber, newInode);
	while (!inputStack.empty())
		inputStack.pop();
	newInode->finode.fileSize = newInode->finode.fileSize - 512 + sum;
	//Direct* sonDir = new Direct();
	//sonDir->directEntryNum = 0;
	//virtualDisk.seekg(getBlockPos(newInode->finode.addr[0]), ios::beg);
	//virtualDisk.write((const char*)sonDir, sizeof(Direct));
	
	synchronizationInode(newInode);

	DirectEntry newEntry;
	newEntry.inodeID = newInode->inodeID;
	strcpy(newEntry.directEntryName, filename);

	dir->directEntry[dir->directEntryNum] = newEntry;
	dir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.write((const char*)dir, sizeof(Direct));
	return 1;
}

int cat_(char* filename) {
	//cout << filename;
	if (strcmp(currentUser->username, "root") != 0) {
		if (strcmp(currentUser->username, currentInode.finode.owner) == 0) {
			int ownerRight = getOwnerRight(currentInode.finode.mode);
			if (!hasWrite(ownerRight)) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
		else if (strcmp(currentUser->groupname, currentInode.finode.group) == 0) {
			int groupRight = getGroupRight(currentInode.finode.mode);
			if (!hasWrite(groupRight)) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
		else {
			int othersRight = getOthersRight(currentInode.finode.mode);
			if (!hasWrite(othersRight)) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
	}
	int sumAddrNumber = currentInode.finode.fileSize / 512;
	int freeAddrNumber = -1;
	int flag = false;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			if (tempDir->directEntryNum < 31 && flag == false) {
				freeAddrNumber = i + 1;
				flag = true;
			}
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					cout << "Can't create file" << filename << ":file exists" << endl;
					return FILE_EXIST;
				}
			}
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = currentInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			Direct* tempDir = new Direct();
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			if (tempDir->directEntryNum < 31 && flag == false) {
				freeAddrNumber = i + 5;
				flag = true;
			}
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					cout << "Can't create file" << filename << ":file exists" << endl;
					return FILE_EXIST;
				}
			}
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = currentInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax = 128;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				Direct* tempDir = new Direct();
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				if (tempDir->directEntryNum < 31 && flag == false) {
					freeAddrNumber = 128 + 4 + 128 * x + i + 1;
					flag = true;
				}
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
						cout << "Can't create file" << filename << ":file exists" << endl;
						return FILE_EXIST;
					}
				}
			}
		}

	}

	int targetFreeBlockID;
	if (freeAddrNumber == -1) {
		if (sumAddrNumber < 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[addrNumber] = targetFreeBlockID;
		}
		else if (sumAddrNumber == 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[4] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[4]), ios::beg);
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber < 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int addr4 = currentInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (addrNumber - 4), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber == 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[5] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]), ios::beg);
			int secondBlock = getFreeBlockID();
			virtualDisk.write((const char*)&secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int secondPos = (addrNumber - 133) / 128 + 1;
			int firstPos = ((addrNumber - 132) % 128) == 0 ? 128 : ((addrNumber - 132) % 128);
			if (firstPos == 128) {
				int newSecondBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * secondPos, ios::beg);
				virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				secondPos++;
				firstPos = 0;

			}
			int secondBlock=-1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (secondPos - 1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (firstPos), sizeof(int));
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));


		}
		Direct* newDir = new Direct();
		newDir->directEntryNum = 0;
		virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
		virtualDisk.write((const char*)newDir, sizeof(Direct));
		currentInode.finode.fileSize += 512;
		synchronizationInode(&currentInode);
	}
	else {
		if (freeAddrNumber <= 4) {
			targetFreeBlockID = currentInode.finode.addr[freeAddrNumber - 1];
		}
		else if (freeAddrNumber <= 132) {
			int addr4 = currentInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (freeAddrNumber - 5), ios::beg);
			virtualDisk.read((char*)&targetFreeBlockID, sizeof(int));
		}
		else {
			int addr[3];
			numberToPos(freeAddrNumber, addr);
			int secondBlock=-1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (addr[1] - 1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (addr[2] - 1), ios::beg);
			virtualDisk.read((char*)&targetFreeBlockID, sizeof(int));
		}

	}
	//insertDirectEntry(targetFreeBlockID, filename);
	putContentToNewFile(targetFreeBlockID, filename);

	return 1;
}



int cat__(char* filename) {
	int targetFreeBlockID = -1;
	int targetInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (targetInodeID == -1) {
		cout << "cat:" << filename << ":no such file or directory" << endl;
		return -1;
	}
	Inode targetInode = getInode(targetInodeID);
	if (targetInode.finode.mode / 1000 == 1) {
		cout << "cat:" << filename << ":is a directory" << endl;
		return -1;
	}
	if (strcmp(currentUser->username, "root") != 0) {
		if (strcmp(currentUser->username, targetInode.finode.owner) == 0) {
			int ownerRight = getOwnerRight(targetInode.finode.mode);
			if (!hasWrite(ownerRight)) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
		else if (strcmp(currentUser->groupname, targetInode.finode.group) == 0) {
			int groupRight = getGroupRight(targetInode.finode.mode);
			if (!hasWrite(groupRight)) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
		else {
			int othersRight = getOthersRight(targetInode.finode.mode);
			if (!hasWrite(othersRight)) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
	}
	time_t timer;
	time(&timer);
	targetInode.finode.modifiedTime = timer;
	queue<string>inputStack;
	int writeNumber = (targetInode.finode.fileSize) / 512 + 1;
	int sum = 0;
	char* temp = new char[512];
	if (targetInode.finode.fileSize % 512 != 0) {
		int addr[3];
		int currentBlock;
		int result = numberToPos(writeNumber, addr);
		if (result == 1) {
			currentBlock = targetInode.finode.addr[writeNumber - 1];
		}
		else if (result == 2) {
			virtualDisk.seekg(getBlockPos(targetInode.finode.addr[4]) + sizeof(int) * (addr[1] - 1), ios::beg);
			virtualDisk.read((char*)&currentBlock, sizeof(int));
		}
		else if (result == 3) {
			int secondAddrTemp;
			virtualDisk.seekg(getBlockPos(targetInode.finode.addr[5]) + sizeof(int) * (addr[1] - 1), ios::beg);
			virtualDisk.read((char*)&secondAddrTemp, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondAddrTemp) + sizeof(int) * (addr[2] - 1), ios::beg);
			virtualDisk.read((char*)&currentBlock, sizeof(int));
		}

		sum = targetInode.finode.fileSize % 512;
		int begin = sum;
		bool gt512 = false;
		string remainString;
		while (cin.getline(temp,512)) {
			string tempString(temp);
			tempString.push_back('\n');
			sum += tempString.length();
			if (sum <= 512) {
				//string tempString(temp);
				inputStack.push(tempString);
			}
			else {
				gt512 = true;
				inputStack.push(tempString.substr(0, tempString.length() - (sum - 512)));
				remainString = tempString.substr(tempString.length() - (sum - 512), sum-512);
				break;
			}
		}
		virtualDisk.seekg(getBlockPos(currentBlock) + begin, ios::beg);
		while (!inputStack.empty()) {
			string str1 = inputStack.front();
			targetInode.finode.fileSize += str1.length();
			const char* str2 = str1.data();
			virtualDisk.write(str2, str1.length());
			inputStack.pop();
		}
		sum = 0 + sum - 512;
		writeNumber++;
		if (!gt512) {
			synchronizationInode(&targetInode);
			return 1;
		}
		inputStack.push(remainString);
		
	}
	while (cin.getline(temp,512))
	{
		string tempString(temp);
		tempString.push_back('\n');
		sum += tempString.length();
		if (sum <= 512) {
			//string tempString(temp);
			inputStack.push(tempString);
		}
		else
		{
			inputStack.push(tempString.substr(0, tempString.length() - (sum - 512)));
			putContenToBlock(inputStack, writeNumber, &targetInode);
			while (!inputStack.empty())
				inputStack.pop();
			sum = 0 + sum - 512;
			writeNumber++;
			inputStack.push(tempString.substr(tempString.length() - sum, sum));
		}
	}
	if (!inputStack.empty()) {
		putContenToBlock(inputStack, writeNumber, &targetInode);
		while (!inputStack.empty())
			inputStack.pop();
		targetInode.finode.fileSize = targetInode.finode.fileSize - 512 + sum;
	}
	synchronizationInode(&targetInode);
	return 1;
}

int cat(char* filename) {
	int targetFreeBlockID = -1;
	int targetInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (targetInodeID == -1) {
		cout << "cat:" << filename << ":no such file or directory" << endl;
		return -1;
	}
	Inode targetInode = getInode(targetInodeID);
	if (targetInode.finode.mode / 1000 == 1) {
		cout << "cat:" << filename << ":is a directory" << endl;
		return -1;
	}
	if (strcmp(currentUser->username, "root") != 0) {
		if (strcmp(currentUser->username, targetInode.finode.owner) == 0) {
			int ownerRight = getOwnerRight(targetInode.finode.mode);
			if (ownerRight < 4) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
		else if (strcmp(currentUser->groupname, targetInode.finode.group) == 0) {
			int groupRight = getGroupRight(targetInode.finode.mode);
			if (groupRight < 4) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
		else {
			int othersRight = getOthersRight(targetInode.finode.mode);
			if (othersRight < 4) {
				cout << "Permission Denied" << endl;
				return -1;
			}
		}
	}

	int sumAddrNumber = (targetInode.finode.fileSize==0)?0:((targetInode.finode.fileSize-1)/ 512+1);
	int remainChar;
	if (targetInode.finode.fileSize == 0)remainChar = 0;
	else remainChar=(targetInode.finode.fileSize%512==0)?512: targetInode.finode.fileSize % 512;
	char *buffer=new char[512];
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			virtualDisk.seekg(getBlockPos(targetInode.finode.addr[i]), ios::beg);
			if (i == (sumAddrNumber - 1)) {
				memset(buffer, 0, 512);
				virtualDisk.read((char*)buffer, remainChar);
			}
			else
			{
				virtualDisk.read((char*)buffer, 512);
			}
			
			
			cout << buffer;

		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = targetInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock=-1;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			if ((i+4)== (sumAddrNumber - 1)) {
				memset(buffer, 0, 512);
				virtualDisk.read((char*)buffer, remainChar);
			}
			else
			{
				virtualDisk.read((char*)buffer, 512);
			}
			cout << buffer;
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = targetInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax = 128;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				if ((132+x*128+i)==(addrNumber-1)) {
					memset(buffer, 0, 512);
					virtualDisk.read((char*)buffer, remainChar);
				}
				else
				{
					virtualDisk.read((char*)buffer, 512);
				}
				cout << buffer;
			}
		}

	}

	return 1;
}

int rm(char* filename) {
	int targetFreeBlockID;
	int targetInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (targetInodeID == -1) {
		cout << "can not remove" << filename << ":No such file" << endl;
	}
	Inode targetInode = getInode(targetInodeID);
	if (targetInode.finode.mode / 1000 == 1) {
		cout << "can not remove:" << filename << " is a direcoty" << endl;
		return 1;
	}
	if (targetInode.finode.fileSize == 0) {}
	else if (targetInode.finode.fileLink > 1) {}
	else
	{
		int tempaddrNumber = (targetInode.finode.fileSize - 1) / 512 + 1;
		if (tempaddrNumber > 0) {
			int temptempAddrNumber = (tempaddrNumber > 4) ? 4 : tempaddrNumber;
			for (int k = 0;k < temptempAddrNumber;k++) {
				blockFree(targetInode.finode.addr[k]);
			}
		}
		if (tempaddrNumber > 4) {
			int temptempAddrNumber = (tempaddrNumber > 132) ? 132 : tempaddrNumber;
			int addr4 = targetInode.finode.addr[4];
			for (int x = 0;x < temptempAddrNumber - 4;x++) {
				virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * x, ios::beg);
				int currentBlock;
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				blockFree(currentBlock);
			}
			blockFree(addr4);
		}
		if (tempaddrNumber > 132) {}
	}
	if (targetInode.finode.fileLink > 1) {
		targetInode.finode.fileLink--;
		synchronizationInode(&targetInode);
	}
	else {
		targetInode.finode.fileLink = 0;
		targetInode.finode.fileSize == 0;
		synchronizationInode(&targetInode);
		super->freeTotalInode++;
		synchronizationSuperBlock();
	}
	


	int sumAddrNumber = currentInode.finode.fileSize / 512;
	Inode rmInode;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					rmInode = getInode(tempDir->directEntry[j].inodeID);
					for (int k = j;k < tempDir->directEntryNum;k++)
						tempDir->directEntry[k] = tempDir->directEntry[k + 1];
					tempDir->directEntryNum--;
					virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
					virtualDisk.write((const char*)tempDir, sizeof(Direct));
					return 1;
				}
			}
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = currentInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					rmInode = getInode(tempDir->directEntry[j].inodeID);
					for (int k = j;k < tempDir->directEntryNum;k++)
						tempDir->directEntry[k] = tempDir->directEntry[k + 1];
					tempDir->directEntryNum--;
					virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
					virtualDisk.write((const char*)tempDir, sizeof(Direct));
					return 1;
				}
			}
		}
	}
	if (sumAddrNumber > 132 ) {
		int addrNumber = sumAddrNumber;
		int addr5 = currentInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				Direct* tempDir = new Direct();
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
						rmInode = getInode(tempDir->directEntry[j].inodeID);
						for (int k = j;k < tempDir->directEntryNum;k++)
							tempDir->directEntry[k] = tempDir->directEntry[k + 1];
						tempDir->directEntryNum--;
						virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
						virtualDisk.write((const char*)tempDir, sizeof(Direct));
						return 1;
					}
				}
			}
		}

	}
	return 1;
}

int chmod(char *modeStr,char* filename) {
	int targetFreeBlockID;
	int targetInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (targetInodeID == -1){
		cout << "chmod " << filename << "failed:no such file or directory" << endl;
		return -1;
	}
	Inode targetInode = getInode(targetInodeID);
	
	if (modeStr[0] < '0' || modeStr[0]>'9' || modeStr[1] < '0' || modeStr[1]>'9' || modeStr[2] < '0' || modeStr[2]>'9') {
		cout << "chmod " << filename << "failed:error mode style" << endl;
		return -1;
	}
	int mode = 0;
	mode += ((modeStr[0] - '0') > 7 ? 7 : (modeStr[0] - '0')) * 100 + ((modeStr[1] - '0') > 7 ? 7 : (modeStr[1] - '0')) * 10 + ((modeStr[2] - '0') > 7 ? 7 : (modeStr[2] - '0'));
	targetInode.finode.mode = (targetInode.finode.mode / 1000) * 1000 + mode;
	synchronizationInode(&targetInode);
	
	return 1;
}

int chgrp(char* groupname,char *filename)
{
	if (strcmp(currentUser->username, "root") != 0)
	{
		cout << "Access deny!" << endl;
		return -1;
	}
	int targetFreeBlockID;
	int targetInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (targetInodeID == -1) {
		cout << "chgrp failed:" << filename << "does not exits" << endl;
		return -1;
	}
	bool existUser = false;
	for (int i = 0;i < usernum;i++) {
		if (strcmp(users[i].groupname, groupname) == 0) {
			existUser = true;
			break;
		}
	}
	if (!existUser) {
		cout << "Non existing group" << endl;
		return -1;
	}
	Inode targetInode = getInode(targetInodeID);
	strcpy(targetInode.finode.group,groupname);
	synchronizationInode(&targetInode);
	return 1;
}

int chown(char* ownername, char* filename) {
	if (strcmp(currentUser->username, "root") != 0)
	{
		cout << "Access deny!" << endl;
		return -1;
	}
	int targetFreeBlockID;
	int targetInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (targetInodeID == -1) {
		cout << "chown failed:" << filename << "does not exits" << endl;
		return -1;
	}
	bool existUser = false;
	for (int i = 0;i < usernum;i++) {
		if (strcmp(users[i].username, ownername) == 0) {
			existUser = true;
			break;
		}
	}
	if (!existUser) {
		cout << "Non existing users" << endl;
		return -1;
	}
	Inode targetInode = getInode(targetInodeID);
	strcpy(targetInode.finode.owner, ownername);
	synchronizationInode(&targetInode);
	return 1;
}

int umask(char* umask) {
	if (strcmp(currentUser->username, "root") != 0)
	{
		cout << "Access deny!" << endl;
		return -1;
	}
	if (umask == NULL) {
		
		for (int i = 0;i < 4;i++) {
			cout << (int)super->umask[i];
		}
		cout << endl;
	}
	else
	{
		if (strlen(umask) == 3) {
			if (umask[0] >= '0' && umask[0] < '8' && umask[1]>='0' && umask[1] < '8' && umask[2] >='0' && umask[2] < '8') {
				super->umask[1] = umask[0]-'0';
				super->umask[2] = umask[1] - '0';
				super->umask[3] = umask[2] - '0';
				synchronizationSuperBlock();
				return 1;
			}
		}
		cout << "umask failed:" << "erro umask style" << endl;
	}
	return 1;
}

int cp(char* filename, char* path) {
	//cout << "filename:" << filename << endl;
	//cout << path << endl;
	if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
		cout << "rm:failed to remove " << filename << ":Device or resource busy";
		return -1;
	}
	int targetFreeBlockID;
	int sourceInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (sourceInodeID == -1) {
		cout << "can not find file" << filename << endl;
		return -1;
	}
	char* dirNames[20];
	Inode tempCurrentInode;
	if (path[0] == '/') {
		tempCurrentInode = *root;
	}
	else {
		tempCurrentInode = currentInode;
	}
	//cout << "cd start" << endl;
	dirNames[0] = strtok(path, "/");
	int i = 0;
	while (dirNames[i] != NULL)
	{
		i++;
		dirNames[i] = strtok(NULL, "/");
	}
	dirNames[i] = "\0";
	/*for (int j = 0; j < i; j++)
	{
		cout << dirNames[j] << endl;
	}*/
	int targetInodeID;
	for (int j = 0;j < i;j++) {
			int targetFreeBlockID;
			targetInodeID = findFileInDir(tempCurrentInode, dirNames[j], targetFreeBlockID);
			if (targetInodeID == -1) {
				cout << "can not find directory" << dirNames[j] << endl;
				return -1;
			}
			tempCurrentInode = getInode(targetInodeID, tempCurrentInode.inodeID);
			if (tempCurrentInode.finode.mode / 1000 != 1) {
				cout << dirNames[j] << "is not a directory" << endl;
				return -1;
			}
	}
	Inode destinationInode = tempCurrentInode;
	targetFreeBlockID = -1;
	int existFileID = findFileInDir(destinationInode, filename, targetFreeBlockID);
	if (existFileID != -1) {
		cout << "file" << filename << "has existed in target directory" << endl;
		return -1;
	}
	if (targetFreeBlockID == -1) {
		int sumAddrNumber = destinationInode.finode.fileSize / 512;
		if (sumAddrNumber < 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			destinationInode.finode.addr[addrNumber] = targetFreeBlockID;
		}
		else if (sumAddrNumber == 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			destinationInode.finode.addr[4] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(destinationInode.finode.addr[4]), ios::beg);
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber < 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int addr4 = destinationInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (addrNumber - 4), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber == 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[5] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]), ios::beg);
			int secondBlock = getFreeBlockID();
			virtualDisk.write((const char*)&secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int secondPos = (addrNumber - 133) / 128 + 1;
			int firstPos = ((addrNumber - 132) % 128) == 0 ? 128 : ((addrNumber - 132) % 128);
			if (firstPos == 128) {
				int newSecondBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * secondPos, ios::beg);
				virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				secondPos++;
				firstPos = 0;

			}
			int secondBlock=-1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (secondPos - 1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (firstPos), sizeof(int));
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));


		}
		Direct* newDir = new Direct();
		newDir->directEntryNum = 0;
		virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
		virtualDisk.write((const char*)newDir, sizeof(Direct));
		destinationInode.finode.fileSize += 512;
		synchronizationInode(&destinationInode);
	}

	Direct* dir = new Direct();
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.read((char*)dir, sizeof(Direct));

	int freeInodeID = getFreeInodeID();
	Inode freeInode = getInode(freeInodeID,destinationInode.inodeID);
	initNode(&freeInode);
	Inode sourceInode = getInode(sourceInodeID);
	freeInode.finode.mode = sourceInode.finode.mode;
	freeInode.finode.fileSize = sourceInode.finode.fileSize;

	int sumAddrNumber =(sourceInode.finode.fileSize==0)?0:(sourceInode.finode.fileSize-1)/ 512+1;
	char * buffer = new char[512];
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			virtualDisk.seekg(getBlockPos(sourceInode.finode.addr[i]), ios::beg);
			virtualDisk.read(buffer, 512);
			freeInode.finode.addr[i] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(freeInode.finode.addr[i]), ios::beg);
			virtualDisk.write((const char*)buffer, 512);
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = sourceInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			virtualDisk.read(buffer, 512);
			if (i == 0) {
				freeInode.finode.addr[4] = getFreeBlockID();
			}
			int newBlock = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(freeInode.finode.addr[4])+sizeof(int)*i, ios::beg);
			virtualDisk.write((const char*)&newBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(newBlock), ios::beg);
			virtualDisk.write((const char*)buffer, 512);
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = sourceInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax = 128;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				virtualDisk.read(buffer, 512);
				if (x == 0) {
					freeInode.finode.addr[5] = getFreeBlockID();
				}
				if (i == 0) {
					int newSecondBlock = getFreeBlockID();
					virtualDisk.seekg(getBlockPos(freeInode.finode.addr[5]) + sizeof(int) * x, ios::beg);
					virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				}
				int newSecondBlock;
				virtualDisk.seekg(getBlockPos(freeInode.finode.addr[5]) + sizeof(int) * x, ios::beg);
				virtualDisk.read(( char*)&newSecondBlock, sizeof(int));
				int newBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(newSecondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.write((const char*)&newBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(newBlock), ios::beg);
				virtualDisk.write((const char*)buffer, 512);
			}
		}

	}

	synchronizationInode(&freeInode);

	DirectEntry newEntry;
	newEntry.inodeID = freeInodeID;
	strcpy(newEntry.directEntryName, filename);

	dir->directEntry[dir->directEntryNum] = newEntry;
	dir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.write((const char*)dir, sizeof(Direct));
	return 1;
}

int mv(char* filename, char* path) {

	if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
		cout << "rmdir:failed to remove " << filename << ":Device or Resource busy";
		return -1;
	}
	int targetFreeBlockID;
	int sourceInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (sourceInodeID == -1) {
		cout << "can not find file" << filename << endl;
		return -1;
	}
	char* dirNames[20];
	Inode tempCurrentInode;
	if (path[0] == '/') {
		tempCurrentInode = *root;
	}
	else {
		tempCurrentInode = currentInode;
	}
	//cout << "cd start" << endl;
	dirNames[0] = strtok(path, "/");
	int i = 0;
	while (dirNames[i] != NULL)
	{
		i++;
		dirNames[i] = strtok(NULL, "/");
	}
	dirNames[i] = "\0";
	/*for (int j = 0; j < i; j++)
	{
		cout << dirNames[j] << endl;
	}*/
	int targetInodeID;
	for (int j = 0;j < i-1;j++) {
		int targetFreeBlockID;
		targetInodeID = findFileInDir(tempCurrentInode, dirNames[j], targetFreeBlockID);
		if (targetInodeID == -1) {
			cout << "can not find directory" << dirNames[j] << endl;
			return -1;
		}
		tempCurrentInode = getInode(targetInodeID, tempCurrentInode.inodeID);
		if (tempCurrentInode.finode.mode / 1000 != 1) {
			cout << dirNames[j] << "is not a directory" << endl;
			return -1;
		}
	}
	char* targetname = dirNames[i - 1];
	Inode destinationInode = tempCurrentInode;
	targetFreeBlockID = -1;
	int existFileID = findFileInDir(destinationInode, targetname, targetFreeBlockID);
	if (existFileID != -1) {
		cout << "file" << targetname << "has existed in target directory" << endl;
		return -1;
	}
	int sumAddrNumber = currentInode.finode.fileSize / 512;
	int freeAddrNumber = -1;
	bool flag = false;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					for (int k = j;k < tempDir->directEntryNum;k++)
						tempDir->directEntry[k] = tempDir->directEntry[k + 1];
					tempDir->directEntryNum--;
					virtualDisk.seekg(getBlockPos(currentInode.finode.addr[i]), ios::beg);
					virtualDisk.write((const char*)tempDir, sizeof(Direct));
					flag = true;
					break;
				}
			}
			if (flag)break;
		}
	}
	if (sumAddrNumber > 4 && flag == false) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = currentInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
					for (int k = j;k < tempDir->directEntryNum;k++)
						tempDir->directEntry[k] = tempDir->directEntry[k + 1];
					tempDir->directEntryNum--;
					virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
					virtualDisk.write((const char*)tempDir, sizeof(Direct));
					flag = true;
					break;
				}
			}
			if (flag)break;
		}
	}
	if (sumAddrNumber > 132 && flag == false) {
		int addrNumber = sumAddrNumber;
		int addr5 = currentInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				Direct* tempDir = new Direct();
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				virtualDisk.read((char*)tempDir, sizeof(Direct));
				for (int j = 0;j < tempDir->directEntryNum;j++) {
					if (strcmp((tempDir->directEntry[j].directEntryName), filename) == 0) {
						for (int k = j;k < tempDir->directEntryNum;k++)
							tempDir->directEntry[k] = tempDir->directEntry[k + 1];
						tempDir->directEntryNum--;
						virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
						virtualDisk.write((const char*)tempDir, sizeof(Direct));
						flag = true;
						break;
					}
				}
				if (flag)break;
			}
		}

	}
	
	sumAddrNumber = tempCurrentInode.finode.fileSize / 512;
	if (targetFreeBlockID == -1) {
		if (sumAddrNumber < 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[addrNumber] = targetFreeBlockID;
		}
		else if (sumAddrNumber == 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[4] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[4]), ios::beg);
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber < 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int addr4 = currentInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (addrNumber - 4), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber == 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[5] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]), ios::beg);
			int secondBlock = getFreeBlockID();
			virtualDisk.write((const char*)&secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int secondPos = (addrNumber - 133) / 128 + 1;
			int firstPos = ((addrNumber - 132) % 128) == 0 ? 128 : ((addrNumber - 132) % 128);
			if (firstPos == 128) {
				int newSecondBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * secondPos, ios::beg);
				virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				secondPos++;
				firstPos = 0;

			}
			int secondBlock = -1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (secondPos - 1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (firstPos), sizeof(int));
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));


		}
		Direct* newDir = new Direct();
		newDir->directEntryNum = 0;
		virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
		virtualDisk.write((const char*)newDir, sizeof(Direct));
		currentInode.finode.fileSize += 512;
		synchronizationInode(&currentInode);
	}

	Direct* destinationDir = new Direct;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.read((char*)destinationDir, sizeof(Direct));
	strcpy(destinationDir->directEntry[destinationDir->directEntryNum].directEntryName, targetname);
	destinationDir->directEntry[destinationDir->directEntryNum].inodeID = sourceInodeID;
	destinationDir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.write((const char*)destinationDir, sizeof(Direct));

	Inode sourceInode = getInode(sourceInodeID);
	time_t timer;
	time(&timer);
	sourceInode.finode.modifiedTime = timer;
	synchronizationInode(&sourceInode);
	if (sourceInode.finode.mode / 1000 == 1) {
		Direct* firstDir = new Direct();
		virtualDisk.seekg(getBlockPos(sourceInode.finode.addr[0]), ios::beg);
		virtualDisk.read((char*)firstDir, sizeof(Direct));
		firstDir->directEntry[1].inodeID = destinationInode.inodeID;
		virtualDisk.seekg(getBlockPos(sourceInode.finode.addr[0]), ios::beg);
		virtualDisk.write((const char*)firstDir, sizeof(Direct));
	}

	return 1;
	

}

int ln(char* filename, char* path) {
	//cout << "filename:" << filename << endl;
	//cout << path << endl;
	int targetFreeBlockID;
	int sourceInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (sourceInodeID == -1) {
		cout << "can not find file" << filename << endl;
		return -1;
	}
	char* dirNames[20];
	Inode tempCurrentInode;
	if (path[0] == '/') {
		tempCurrentInode = *root;
	}
	else {
		tempCurrentInode = currentInode;
	}
	//cout << "cd start" << endl;
	dirNames[0] = strtok(path, "/");
	int i = 0;
	while (dirNames[i] != NULL)
	{
		i++;
		dirNames[i] = strtok(NULL, "/");
	}
	dirNames[i] = "\0";
	/*for (int j = 0; j < i; j++)
	{
		cout << dirNames[j] << endl;
	}*/
	int targetInodeID;
	for (int j = 0;j < i-1;j++) {
		int targetFreeBlockID;
		targetInodeID = findFileInDir(tempCurrentInode, dirNames[j], targetFreeBlockID);
		if (targetInodeID == -1) {
			cout << "can not find directory" << dirNames[j] << endl;
			return -1;
		}
		tempCurrentInode = getInode(targetInodeID, tempCurrentInode.inodeID);
		if (tempCurrentInode.finode.mode / 1000 != 1) {
			cout << dirNames[j] << "is not a directory" << endl;
			return -1;
		}
	}
	Inode destinationInode = tempCurrentInode;
	char* lnName = dirNames[i - 1];
	//cout << "lnName:" << lnName << endl;
	targetFreeBlockID = -1;
	int existFileID = findFileInDir(destinationInode, lnName, targetFreeBlockID);
	if (existFileID != -1) {
		cout << "file" << filename << "has existed in target directory" << endl;
		return -1;
	}
	if (targetFreeBlockID == -1) {
		int sumAddrNumber = destinationInode.finode.fileSize / 512;
		if (sumAddrNumber < 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			destinationInode.finode.addr[addrNumber] = targetFreeBlockID;
		}
		else if (sumAddrNumber == 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			destinationInode.finode.addr[4] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(destinationInode.finode.addr[4]), ios::beg);
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber < 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int addr4 = destinationInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (addrNumber - 4), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber == 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[5] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]), ios::beg);
			int secondBlock = getFreeBlockID();
			virtualDisk.write((const char*)&secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int secondPos = (addrNumber - 133) / 128 + 1;
			int firstPos = ((addrNumber - 132) % 128) == 0 ? 128 : ((addrNumber - 132) % 128);
			if (firstPos == 128) {
				int newSecondBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * secondPos, ios::beg);
				virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				secondPos++;
				firstPos = 0;

			}
			int secondBlock=-1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (secondPos - 1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (firstPos), sizeof(int));
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));


		}
		Direct* newDir = new Direct();
		newDir->directEntryNum = 0;
		virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
		virtualDisk.write((const char*)newDir, sizeof(Direct));
		destinationInode.finode.fileSize += 512;
		synchronizationInode(&destinationInode);
	}

	Direct* dir = new Direct();
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.read((char*)dir, sizeof(Direct));

	DirectEntry newEntry;
	newEntry.inodeID = sourceInodeID;
	strcpy(newEntry.directEntryName, lnName);

	dir->directEntry[dir->directEntryNum] = newEntry;
	dir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.write((const char*)dir, sizeof(Direct));

	Inode sourceInode = getInode(sourceInodeID);
	sourceInode.finode.fileLink++;
	synchronizationInode(&sourceInode);
	return 1;

	
}

int loadSuper(char* diskPath) {
	virtualDisk.open(diskPath, ios::in | ios::out | ios::binary);
	if (!virtualDisk.is_open()) {
		cerr << "open virtualDisk failed" << endl;
		return OPEN_FAILED;
	}
	virtualDisk.seekg(0, ios::beg);
	super = new SuperBlock();
	virtualDisk.read((char*)super, sizeof(SuperBlock));
	root = new Inode();
	virtualDisk.seekg(getInodePos(0), ios::beg);
	virtualDisk.read((char*)&(root->finode), sizeof(Finode));
	root->inodeID = 0;
	root->parentInodeID = 0;
	//vistedInode[0] = *root;
	currentInode = *root;


	return OPEN_OK;

}

int append(char* filename) {
	int targetFreeBlockID = -1;
	int targetInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (targetInodeID == -1) {
		cout << "append:" << filename << ":no such file or directory" << endl;
		return -1;
	}
	Inode targetInode = getInode(targetInodeID);
	if (targetInode.finode.mode / 1000 == 1) {
		cout << "append:" << filename << ":is a directory" << endl;
		return -1;
	}
	char* buffer512 = new char[512];
	for (int i = 0;i < 511;i++) {
		buffer512[i] = 'a';
	}
	buffer512[511] = 0;
	queue<string>inputStack;
	int writeNumber = (targetInode.finode.fileSize) / 512 + 1;

	cout << "targetInode.finode.addr[0]" << targetInode.finode.addr[0] << endl;
	int sum = 0;

	char* temp = new char[512];
	if (targetInode.finode.fileSize % 512 != 0) {
		int addr[3];
		int currentBlock=-1;
		int result = numberToPos(writeNumber, addr);
		if (result == 1) {
			currentBlock = targetInode.finode.addr[writeNumber - 1];
		}
		else if (result == 2) {
			virtualDisk.seekg(getBlockPos(targetInode.finode.addr[4]) + sizeof(int) * (addr[1] - 1), ios::beg);
			virtualDisk.read((char*)&currentBlock, sizeof(int));
		}
		else if (result == 3) {
			int secondAddrTemp;
			virtualDisk.seekg(getBlockPos(targetInode.finode.addr[5]) + sizeof(int) * (addr[1] - 1), ios::beg);
			virtualDisk.read((char*)&secondAddrTemp, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondAddrTemp) + sizeof(int) * (addr[2] - 1), ios::beg);
			virtualDisk.read((char*)&currentBlock, sizeof(int));
		}

		virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
		cout << "currentBlock" << currentBlock << endl;
		virtualDisk.write((const char*)&buffer512[0], 512);
		//cout << buffer512 << endl;
		writeNumber++;
		targetInode.finode.fileSize = targetInode.finode.fileSize - (targetInode.finode.fileSize % 512) + 512;
		synchronizationInode(&targetInode);
		return 1;

	}
	
	int addr[3];
	int result = numberToPos(writeNumber, addr);
	int newBlock = getFreeBlockID();
	targetInode.finode.fileSize += 512;
	if (result == 1) {
		targetInode.finode.addr[writeNumber - 1] = newBlock;
	}
	else if (result == 2) {
		if (writeNumber == 5) {
			targetInode.finode.addr[4] = getFreeBlockID();
		}
		virtualDisk.seekg(getBlockPos(targetInode.finode.addr[4]) + sizeof(int) * (addr[1] - 1), ios::beg);
		virtualDisk.write((const char*)&newBlock, sizeof(int));
	}
	else if (result == 3) {
		if (writeNumber == 133) {
			targetInode.finode.addr[5] = getFreeBlockID();
		}
		if (addr[2] == 1) {
			int secondAddr = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(targetInode.finode.addr[5]) + sizeof(int) * (addr[1] - 1), ios::beg);
			virtualDisk.write((const char*)&secondAddr, sizeof(int));
		}
		int secondAddrTemp;
		virtualDisk.seekg(getBlockPos(targetInode.finode.addr[5]) + sizeof(int) * (addr[1] - 1), ios::beg);
		virtualDisk.read((char*)&secondAddrTemp, sizeof(int));
		virtualDisk.seekg(getBlockPos(secondAddrTemp) + sizeof(int) * (addr[2] - 1), ios::beg);
		virtualDisk.write((const char*)&newBlock, sizeof(int));
	}
	virtualDisk.seekg(getBlockPos(newBlock), ios::beg);
	virtualDisk.write((const char*)buffer512, 512);
	synchronizationInode(&targetInode);
	return 1;
}

int get(char* inodeID) {
	string strID(inodeID);
	stringstream ss;
	int ID;
	ss << strID;
	ss >> ID;
	cout << ID;
	Inode inode = getInode(ID);
	cout << "addr" << endl;
	for (int i = 0;i < 6;i++) {
		cout <<"addr["<<i<<"]"<< inode.finode.addr[i] << endl;
	}
	//cout << "parent" << inode.parentInodeID << endl;
	return 1;
}


//需要改进的地方
void insert_Entry(Inode newInode, DirectEntry entry) {
	int targetFreeBlockID = -1;
	findFileInDir(newInode, "$-&fdj", targetFreeBlockID);
	if (targetFreeBlockID == -1) {
		int sumAddrNumber = newInode.finode.fileSize / 512;
		if (sumAddrNumber < 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[addrNumber] = targetFreeBlockID;
		}
		else if (sumAddrNumber == 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[4] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[4]), ios::beg);
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber < 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int addr4 = currentInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (addrNumber - 4), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber == 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[5] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]), ios::beg);
			int secondBlock = getFreeBlockID();
			virtualDisk.write((const char*)&secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int secondPos = (addrNumber - 133) / 128 + 1;
			int firstPos = ((addrNumber - 132) % 128) == 0 ? 128 : ((addrNumber - 132) % 128);
			if (firstPos == 128) {
				int newSecondBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * secondPos, ios::beg);
				virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				secondPos++;
				firstPos = 0;

			}
			int secondBlock = -1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (secondPos - 1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (firstPos), sizeof(int));
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));


		}
		Direct* newDir = new Direct();
		newDir->directEntryNum = 0;
		virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
		virtualDisk.write((const char*)newDir, sizeof(Direct));
		currentInode.finode.fileSize += 512;
		synchronizationInode(&currentInode);
	}

	Direct* dir = new Direct();
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.read((char*)dir, sizeof(Direct));

	dir->directEntry[dir->directEntryNum] = entry;
	dir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.write((const char*)dir, sizeof(Direct));

}

void cp_Node(Inode sourceInode, Inode freeInode) {
	int sumAddrNumber = (sourceInode.finode.fileSize == 0) ? 0 : (sourceInode.finode.fileSize - 1) / 512 + 1;
	char* buffer = new char[512];
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			virtualDisk.seekg(getBlockPos(sourceInode.finode.addr[i]), ios::beg);
			virtualDisk.read(buffer, 512);
			freeInode.finode.addr[i] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(freeInode.finode.addr[i]), ios::beg);
			virtualDisk.write((const char*)buffer, 512);
		}
	}
	if (sumAddrNumber > 4) {
		int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = sourceInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			virtualDisk.read(buffer, 512);
			if (i == 0) {
				freeInode.finode.addr[4] = getFreeBlockID();
			}
			int newBlock = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(freeInode.finode.addr[4]) + sizeof(int) * i, ios::beg);
			virtualDisk.write((const char*)&newBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(newBlock), ios::beg);
			virtualDisk.write((const char*)buffer, 512);
		}
	}
	if (sumAddrNumber > 132) {
		int addrNumber = sumAddrNumber;
		int addr5 = sourceInode.finode.addr[5];
		for (int x = 0;x <= (addrNumber - 133) / 128;x++) {
			virtualDisk.seekg(getBlockPos(addr5) + sizeof(int) * x, ios::beg);
			int secondBlock;
			virtualDisk.read((char*)&secondBlock, sizeof(int));
			int iMax = 128;
			if (x == (addrNumber - 133) / 128) {
				int imax1 = (addrNumber - 132) % 128;
				iMax = (imax1 == 0) ? 128 : imax1;
			}
			for (int i = 0;i < iMax;i++) {
				int currentBlock;
				virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.read((char*)&currentBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
				virtualDisk.read(buffer, 512);
				if (x == 0) {
					freeInode.finode.addr[5] = getFreeBlockID();
				}
				if (i == 0) {
					int newSecondBlock = getFreeBlockID();
					virtualDisk.seekg(getBlockPos(freeInode.finode.addr[5]) + sizeof(int) * x, ios::beg);
					virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				}
				int newSecondBlock;
				virtualDisk.seekg(getBlockPos(freeInode.finode.addr[5]) + sizeof(int) * x, ios::beg);
				virtualDisk.read((char*)&newSecondBlock, sizeof(int));
				int newBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(newSecondBlock) + sizeof(int) * i, ios::beg);
				virtualDisk.write((const char*)&newBlock, sizeof(int));
				virtualDisk.seekg(getBlockPos(newBlock), ios::beg);
				virtualDisk.write((const char*)buffer, 512);
			}
		}

	}

	synchronizationInode(&freeInode);
}

void _cpdir(Inode sourceInode, Inode newInode) {
	int sumAddrNumber = sourceInode.finode.fileSize / 512;
	if (sumAddrNumber > 0) {
		int addrNumber = (sumAddrNumber > 4) ? 4 : sumAddrNumber;
		for (int i = 0;i < addrNumber;i++) {
			Direct* tempDir = new Direct();
			virtualDisk.seekg(getBlockPos(sourceInode.finode.addr[i]), ios::beg);
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				if (strcmp(tempDir->directEntry[j].directEntryName, ".") == 0 || strcmp(tempDir->directEntry[j].directEntryName, "..") == 0) {
					continue;
				}
				Inode tempInode = getInode(tempDir->directEntry[j].inodeID);
				if (tempInode.finode.mode / 1000 == 1) {
					int newnewInodeID = getFreeInodeID();
					Inode newnewInode = getInode(newnewInodeID);
					initNode(&newnewInode);
					newnewInode.finode.fileSize = 512;
					newnewInode.finode.addr[0] = getFreeBlockID();
					newnewInode.finode.mode = tempInode.finode.mode;
					synchronizationInode(&newnewInode);

					Direct* newDir = new Direct();
					DirectEntry pointEntry;
					DirectEntry pointpointEntry;
					strcpy(pointEntry.directEntryName, ".");
					strcpy(pointpointEntry.directEntryName, "..");
					pointEntry.inodeID = newnewInodeID;
					pointpointEntry.inodeID = newInode.inodeID;
					newDir->directEntry[0] = pointEntry;
					newDir->directEntry[1] = pointpointEntry;
					newDir->directEntryNum = 2;
					virtualDisk.seekg(getBlockPos(newnewInode.finode.addr[0]), ios::beg);
					virtualDisk.write((const char*)newDir, sizeof(Direct));

					DirectEntry entry;
					entry.inodeID = newnewInode.inodeID;
					strcpy(entry.directEntryName, tempDir->directEntry[j].directEntryName);
					insert_Entry(newInode, entry);
					_cpdir(tempInode, newnewInode);

				}
				else {
					if (tempInode.finode.fileSize == 0) {}
					else
					{
						int newnewInodeID = getFreeInodeID();
						Inode newnewInode = getInode(newnewInodeID);
						initNode(&newnewInode);
						newnewInode.finode.fileSize = tempInode.finode.fileSize;
						//newnewInode.finode.addr[0] = getFreeBlockID();
						newnewInode.finode.mode = tempInode.finode.mode;
						synchronizationInode(&newnewInode);
						DirectEntry entry;
						entry.inodeID = newnewInode.inodeID;
						strcpy(entry.directEntryName, tempDir->directEntry[j].directEntryName);
						insert_Entry(newInode, entry);
						cp_Node(tempInode, newnewInode);
					}
				}
				/*tempInode.finode.fileLink = 0;
				tempInode.finode.fileSize == 0;
				synchronizationInode(&tempInode);
				super->freeTotalInode++;
				synchronizationSuperBlock();*/
			}
			/*blockFree(sourceInode.finode.addr[i]);*/
		}
	}
	if (sumAddrNumber > 4) {
		/*int addrNumber = (sumAddrNumber > 132) ? 132 : sumAddrNumber;
		int addr4 = sourceInode.finode.addr[4];
		for (int i = 0;i < addrNumber - 4;i++) {
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * i, ios::beg);
			int currentBlock;
			virtualDisk.read((char*)&currentBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(currentBlock), ios::beg);
			Direct* tempDir = new Direct();
			virtualDisk.read((char*)tempDir, sizeof(Direct));
			for (int j = 0;j < tempDir->directEntryNum;j++) {
				Inode tempInode = getInode(tempDir->directEntry[j].inodeID);
				if (tempInode.finode.mode / 1000 == 1) {
					_rmdir(tempInode);
				}
				if (tempInode.finode.fileSize == 0) {}
				else
				{
					int tempaddrNumber = (tempInode.finode.fileSize - 1) / 512 + 1;
					if (tempaddrNumber > 0) {
						int temptempAddrNumber = (tempaddrNumber > 4) ? 4 : tempaddrNumber;
						for (int k = 0;k < temptempAddrNumber;k++) {
							blockFree(tempInode.finode.addr[k]);
						}
					}
					if (tempaddrNumber > 4) {
						int temptempAddrNumber = (tempaddrNumber > 132) ? 132 : tempaddrNumber;
						int addr42 = sourceInode.finode.addr[4];
						for (int x = 0;x < temptempAddrNumber - 4;x++) {
							virtualDisk.seekg(getBlockPos(addr42) + sizeof(int) * x, ios::beg);
							int currentBlock;
							virtualDisk.read((char*)&currentBlock, sizeof(int));
							blockFree(currentBlock);
						}
						blockFree(addr42);
					}
					if (tempaddrNumber > 132) {}
				}
				tempInode.finode.fileLink = 0;
				tempInode.finode.fileSize == 0;
				synchronizationInode(&tempInode);
			}
			blockFree(currentBlock);
		}
		blockFree(addr4);*/
		cout << "竟然用到了";

	}
	if (sumAddrNumber > 132) {}
	return;
}

int cpdir(char* filename, char* path) {
	if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
		cout << "rmdir:failed to remove " << filename << ":Device or Resource busy";
		return -1;
	}
	int targetFreeBlockID;
	int sourceInodeID = findFileInDir(currentInode, filename, targetFreeBlockID);
	if (sourceInodeID == -1) {
		cout << "can not find file" << filename << endl;
		return -1;
	}
	Inode sourceInode = getInode(sourceInodeID);
	if (sourceInode.finode.mode / 1000 != 1) {
		cout << filename << "is not a directory" << endl;
		return -1;
	}
	char* dirNames[20];
	Inode tempCurrentInode;
	if (path[0] == '/') {
		tempCurrentInode = *root;
	}
	else {
		tempCurrentInode = currentInode;
	}
	//cout << "cd start" << endl;
	dirNames[0] = strtok(path, "/");
	int i = 0;
	while (dirNames[i] != NULL)
	{
		i++;
		dirNames[i] = strtok(NULL, "/");
	}
	dirNames[i] = "\0";
	/*for (int j = 0; j < i; j++)
	{
		cout << dirNames[j] << endl;
	}*/
	int targetInodeID;
	for (int j = 0;j < i - 1;j++) {
		int targetFreeBlockID;
		targetInodeID = findFileInDir(tempCurrentInode, dirNames[j], targetFreeBlockID);
		if (targetInodeID == -1) {
			cout << "can not find directory" << dirNames[j] << endl;
			return -1;
		}
		tempCurrentInode = getInode(targetInodeID, tempCurrentInode.inodeID);
		if (tempCurrentInode.finode.mode / 1000 != 1) {
			cout << dirNames[j] << "is not a directory" << endl;
			return -1;
		}
	}
	char* targetname = dirNames[i - 1];
	Inode destinationInode = tempCurrentInode;
	targetFreeBlockID = -1;
	int existFileID = findFileInDir(destinationInode, targetname, targetFreeBlockID);
	if (existFileID != -1) {
		cout << "file" << targetname << "has existed in target directory" << endl;
		return -1;
	}
	int sumAddrNumber = tempCurrentInode.finode.fileSize / 512;
	if (targetFreeBlockID == -1) {
		if (sumAddrNumber < 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[addrNumber] = targetFreeBlockID;
		}
		else if (sumAddrNumber == 4) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[4] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[4]), ios::beg);
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber < 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int addr4 = currentInode.finode.addr[4];
			virtualDisk.seekg(getBlockPos(addr4) + sizeof(int) * (addrNumber - 4), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else if (sumAddrNumber == 132) {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			currentInode.finode.addr[5] = getFreeBlockID();
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]), ios::beg);
			int secondBlock = getFreeBlockID();
			virtualDisk.write((const char*)&secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock), ios::beg);
			virtualDisk.write((const char*)targetFreeBlockID, sizeof(int));
		}
		else {
			int addrNumber = sumAddrNumber;
			targetFreeBlockID = getFreeBlockID();
			int secondPos = (addrNumber - 133) / 128 + 1;
			int firstPos = ((addrNumber - 132) % 128) == 0 ? 128 : ((addrNumber - 132) % 128);
			if (firstPos == 128) {
				int newSecondBlock = getFreeBlockID();
				virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * secondPos, ios::beg);
				virtualDisk.write((const char*)&newSecondBlock, sizeof(int));
				secondPos++;
				firstPos = 0;

			}
			int secondBlock = -1;
			virtualDisk.seekg(getBlockPos(currentInode.finode.addr[5]) + sizeof(int) * (secondPos - 1), ios::beg);
			virtualDisk.read((char*)secondBlock, sizeof(int));
			virtualDisk.seekg(getBlockPos(secondBlock) + sizeof(int) * (firstPos), sizeof(int));
			virtualDisk.write((const char*)&targetFreeBlockID, sizeof(int));


		}
		Direct* newDir = new Direct();
		newDir->directEntryNum = 0;
		virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
		virtualDisk.write((const char*)newDir, sizeof(Direct));
		currentInode.finode.fileSize += 512;
		synchronizationInode(&currentInode);
	}

	Direct* destinationDir = new Direct;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.read((char*)destinationDir, sizeof(Direct));

	DirectEntry entry2;
	strcpy(entry2.directEntryName, targetname);
	//strcpy(destinationDir->directEntry[destinationDir->directEntryNum].directEntryName, targetname);
	int newInodeID = getFreeInodeID();
	//destinationDir->directEntry[destinationDir->directEntryNum].inodeID = newInodeID;
	entry2.inodeID = newInodeID;
	destinationDir->directEntry[destinationDir->directEntryNum] = entry2;
	destinationDir->directEntryNum++;
	virtualDisk.seekg(getBlockPos(targetFreeBlockID), ios::beg);
	virtualDisk.write((const char*)destinationDir, sizeof(Direct));

	Inode newInode = getInode(newInodeID);

	initNode(&newInode);
	newInode.finode.fileSize = 512;
	newInode.finode.addr[0] = getFreeBlockID();
	newInode.finode.mode = sourceInode.finode.mode;
	synchronizationInode(&newInode);

	Direct *newDir = new Direct();
	DirectEntry pointEntry;
	DirectEntry pointpointEntry;
	strcpy(pointEntry.directEntryName, ".");
	strcpy(pointpointEntry.directEntryName, "..");
	pointEntry.inodeID = newInodeID;
	pointpointEntry.inodeID = tempCurrentInode.inodeID;
	newDir->directEntry[0] = pointEntry;
	newDir->directEntry[1] = pointpointEntry;
	newDir->directEntryNum = 2;
	virtualDisk.seekg(getBlockPos(newInode.finode.addr[0]), ios::beg);
	virtualDisk.write((const char*)newDir, sizeof(Direct));
	_cpdir(sourceInode, newInode);






}



bool login() {
	Direct* rootdir = new Direct();
	virtualDisk.seekg(getBlockPos(root->finode.addr[0]), ios::beg);
	virtualDisk.read((char*)rootdir, sizeof(Direct));
	userInfoInode = getInode(rootdir->directEntry[2].inodeID);
	usernum = userInfoInode.finode.fileSize / sizeof(User);
	users = new User[usernum];
	virtualDisk.seekg(getBlockPos(userInfoInode.finode.addr[0]), ios::beg);
	virtualDisk.read((char*)users, userInfoInode.finode.fileSize);
	//cout << users[0].username << endl;
	char user[OWNER_MAX_NAME] = { 0 };
	char password[MAX_PASSWORD] = { 0 };
	cout << "username:";
	cin >> user;
	char ch;
	for (int i = 0;i < usernum;i++)
	{
		if (strcmp(users[i].username, user) == 0)
		{
			cout << "password:";
			//cin >> password;
			for (int i = 0;i < MAX_PASSWORD;i++)
			{
				ch = getch();
				if (ch == 13)
				{
					break;
				}
				password[i] = ch;
				cout << "*";
			}
			if (strcmp(users[i].password, password) == 0)
			{
				cout << endl;
				currentUser = &users[i];
				userIndex = i;
				time_t timer;
				time(&timer);
				cout << "last login:";
				getTime(super->lastLoginTime);
				cout << endl;
				super->lastLoginTime = timer + 8 * 60 * 60;
				synchronizationSuperBlock();
				return true;
			}
			else
			{
				cout << endl;
				cout << "password is not match!" << endl;
				return false;
			}
		}
	}
	cout << "User does not exist" << endl;
	return false;
}

int readCommend() {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY
		| FOREGROUND_RED | FOREGROUND_GREEN);
	cout << currentUser->username<<"@localhost:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE);
	pwd();
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	cout << "$";
	char commend[500] = { 0 };
	char shell[500] = { 0 };
	char ch = getchar();
	int i = 0;
	if (ch == 10)
		return 0;
	while (ch != 10)
	{
		commend[i] = ch;
		ch = getchar();
		i++;
	}
	strtok(commend, " ");
	strcpy(shell, commend);
	if (strcmp(shell, "cd") == 0) {
		cd(&commend[strlen(shell) + 1]);
	}
	else if (strcmp(shell, "mkdir") == 0) {
		mkdir(&commend[strlen(shell) + 1]);
		//cout << endl;
	}
	else if (strcmp(shell, "rmdir") == 0) {
		rmdir(&commend[strlen(shell) + 1]);
		//cout << endl;
	}
	else if (strcmp(shell, "ls") == 0) {
		ls();
		cout << endl;
	}
	else if (strcmp(shell, "ll") == 0) {
		ll();
		//cout << endl;
	}
	else if (strcmp(shell, "passwd") == 0) {
		password();
		//cout << endl;
	}
	else if (strcmp(shell, "exit") == 0) {
		logout = true;
	}
	else if (strcmp(shell, "superInfo") == 0) {
		superInfo();
	}
	else if (strcmp(shell, "pwd") == 0) {
		pwd();
		cout << endl;
	}
	else if (strcmp(shell, "cat") == 0) {
		int begin = strlen(shell) + 1;
		if (commend[begin] == '>' && commend[begin + 1] == '>') {
			cat__(&commend[begin + 2]);
			cin.clear();
			cin.sync();
		}
		else if (commend[begin] == '>') {
			cat_(&commend[begin + 1]);
			cin.clear();
			cin.sync();
		}
		else
		{
			cat(&commend[begin]);
		}
	}
	else if (strcmp(shell, "rm")==0) {
		rm(&commend[strlen(shell) + 1]);
	}
	else if (strcmp(shell, "chmod") == 0) {
		char* mode = new char[20];
		mode = strtok(NULL, " ");
		//cout << mode << endl;
		//cout << &commend[strlen(shell) + strlen(mode) + 2] << endl;
		chmod(mode, &commend[strlen(shell) + strlen(mode) + 2]);
	}
	else if(strcmp(shell, "chgrp") == 0){
		char* groupName = new char[20];
		groupName = strtok(NULL, " ");
		//cout << &commend[strlen(shell) + strlen(mode) + 2] << endl;
		chgrp(groupName, &commend[strlen(shell) + strlen(groupName) + 2]);
	}
	else if (strcmp(shell, "chown") == 0) {
		char* ownername = new char[20];
		ownername = strtok(NULL, " ");
		//cout << &commend[strlen(shell) + strlen(mode) + 2] << endl;
		chown(ownername, &commend[strlen(shell) + strlen(ownername) + 2]);
	}
	else if (strcmp(shell, "umask") == 0) {
		char* umaskValue = new char[20];
		umaskValue = strtok(NULL, " ");
		umask(umaskValue);
	}
	else if (strcmp(shell, "cp") == 0) {
		char* filename = new char[20];
		filename = strtok(NULL," ");
		cp(filename, &commend[strlen(shell) + strlen(filename) + 2]);
	}
	else if (strcmp(shell, "mv") == 0) {
		char* filename = new char[20];
		filename = strtok(NULL, " ");
		mv(filename, &commend[strlen(shell) + strlen(filename) + 2]);
	}
	else if (strcmp(shell, "ln") == 0) {
		char* filename = new char[20];
		filename = strtok(NULL, " ");
		ln(filename, &commend[strlen(shell) + strlen(filename) + 2]);
	}
	else if (strcmp(shell, "append") == 0) {
		append(&commend[strlen(shell) + 1]);
	}
	else if (strcmp(shell, "get") == 0) {
		get(&commend[strlen(shell) + 1]);
	}
	else if (strcmp(shell, "cpdir") == 0) {
		char* filename = new char[20];
		filename = strtok(NULL, " ");
		cpdir(filename, &commend[strlen(shell) + strlen(filename) + 2]);
	}
	else
	{
		return 1;
	}
	return 1;
	

}











