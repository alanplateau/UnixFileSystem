// UnixFileSystem.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// UnixFileSystem.cpp : 此文件包含 "main" 函数。
//

#include <iostream>
#include "fileSystemImplements.h"
using namespace std;

int main()
{
	cout << "UnixFileSystem" << endl;
	cout << "copy right @ 高源" << endl;
	cout << "学号:201706062013" << endl;
	cout << "e-mail:201706062013@zjut.edu.cn" << endl << endl;
	loadSuper("virtualDisk.dat");
	while(!login()) {
		NULL;
	}
	cin.ignore();
	while (!logout)
	{
		readCommend();
	}

}

