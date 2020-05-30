// server1.0.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<afx.h>
#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展
#include <afxdisp.h>        // MFC 自动化类
#include "SYS.h"
#include <CString>


BOOL DeleteDirectory(char* DirName);
typedef struct {
    int ID;
    BYTE lparam[2048];
}COMMAND;

typedef struct {
    char FileName[MAX_PATH];
    int FileLen;
    char Time[50];
    BOOL IsDir;
    BOOL Error;
    HICON hIcon;
}FILEINFO;

DWORD WINAPI SLisen(LPVOID lparam);
DWORD GetDriverProc(COMMAND command, SOCKET client);
DWORD GetDirInfoProc(COMMAND command, SOCKET client);
DWORD ExecFileProc(COMMAND command, SOCKET client);
DWORD GetFileProc(COMMAND command, SOCKET client);
DWORD PutFileProc(COMMAND command, SOCKET client);
DWORD FileInfoProc(COMMAND command, SOCKET client);
DWORD CreateDirProc(COMMAND command, SOCKET client);
DWORD DelDirProc(COMMAND command, SOCKET client);
DWORD ShutDownProc(COMMAND command, SOCKET client);
using namespace std;



int main()
{
    WSADATA wsadata;
    WORD ver = MAKEWORD(2,2);
    WSAStartup(ver, &wsadata);

    //创建套接字
    SOCKET server;
    server = socket(AF_INET, SOCK_STREAM, 0);//套接字属性
    SOCKADDR_IN serveraddr;//服务端属性
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8080);
    serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//接收所有客户端IP

    bind(server, (SOCKADDR*)&serveraddr, sizeof(serveraddr));//捆绑（本地地址和套接字连接）
    listen(server,10);//监听来自客户端的连接

    SOCKET client;
    char password[100];
    while (true) {
        client = accept(server, 0, 0);
ag:     if (client != INVALID_SOCKET) {//服务端接收客户端的连接
            if (send(client, "Password", sizeof("Password"), 0) != SOCKET_ERROR)
                cout << "有客户请求连接，正等待客户输入密码！\n";
            if (recv(client, password, sizeof(password), 0) == SOCKET_ERROR) {
                cout << "无法接受客户端密码！\n";
                return 0;
            }
            else {
                if (strcmp(password, "19980504") == 0) {
                    send(client, "TRUE", sizeof("TRUE"), 0);
                    cout << "用户输入密码正确！\n";
                    CreateThread(NULL,0,SLisen,(LPVOID)client,0,NULL);
                }
                else {
                    send(client, "FALSE", sizeof("FALSE"), 0);
                    cout << "用户输入密码不正确！\n";
                    goto ag;
                }
            }
        }
    }
    closesocket(server);
    closesocket(client);
    WSACleanup();

}

DWORD WINAPI SLisen(LPVOID lparam) {
    SOCKET client = (SOCKET)lparam;
    COMMAND command;
    while (1) {
        memset((char*)&command, 0, sizeof(command));
        if (recv(client, (char*)&command, sizeof(command), 0) == SOCKET_ERROR) {
            cout << "接收客户端指令错误";
            break;
        }
        else {
            switch (command.ID) {
            case GetDriver:
                GetDriverProc(command, client);//得到磁盘信息并返回客户端
                break;
            case GetDirInfo:
                GetDirInfoProc(command, client);//得到文件信息并返回客户端
                break;
            case ExecFile:
                ExecFileProc(command, client);//得到文件路径并执行
                break;
            case GetFile:
                GetFileProc(command, client);//下载文件并返回
                break;
            case PutFile:
                PutFileProc(command, client);//上传文件并返回
                break;
            case GetFileInfo:
                FileInfoProc(command, client);
                break;
            case CreateDir:
                CreateDirProc(command, client);
                break;
            case DelDir:
                DelDirProc(command, client);
                break;
            case ShutDown:
                ShutDownProc(command, client);
                break;

            }
        }

    }
    closesocket(client);
    return 0;
}

DWORD GetDriverProc(COMMAND command, SOCKET client) {
    COMMAND cmd;
    memset((char*)&cmd, 0, sizeof(cmd));
    cmd.ID = GetDriver;
    for (char i = 'A'; i <= 'Z'; i++) {
        char x[20] = { i,':','\0' };
        UINT Type = GetDriveType(x);
        if (Type == DRIVE_FIXED || Type == DRIVE_REMOVABLE || Type == DRIVE_CDROM) {
            memset((char*)cmd.lparam, 0, sizeof(cmd.lparam));
            strcpy_s((char*)cmd.lparam,sizeof(x), x);
            if (send(client, (char*)&cmd, sizeof(cmd), 0) == SOCKET_ERROR) {
                cout << "给客户端返回信息失败!\n";
            }
        }

    }
    return 0;
}

DWORD GetDirInfoProc(COMMAND command, SOCKET client) {
    cout << "接收到客户端请求"<<endl;
    FILEINFO fi;
    COMMAND cmd;
    memset((char*)&fi, 0, sizeof(fi));
    memset((char*)&cmd, 0, sizeof(cmd));
    char pstr[4] = "*.*";
    strcat_s((char*)command.lparam, 100,pstr);

    CFileFind file;
    BOOL bContinue;
    bContinue = file.FindFile((char*)command.lparam,0);
    cout << bContinue << endl;
    cout << command.lparam << endl;
    while (bContinue) {
        memset((char*)&fi, 0, sizeof(fi));
        memset((char*)&cmd, 0, sizeof(cmd));
        bContinue = file.FindNextFile();
        if (file.IsDirectory()) {
            fi.IsDir = true;
        }
        strcpy_s(fi.FileName, MAX_PATH,file.GetFileName().LockBuffer());
        cmd.ID = GetDirInfo;
        memcpy((char*)&cmd.lparam, (char*)&fi, sizeof(fi));
        if (send(client, (char*)&cmd, sizeof(cmd), 0) == SOCKET_ERROR) {
            cout << "向客户端发送文件和文件夹信息失败！" << endl;
        }
        else {
            cout << "向客户端发送文件和文件夹信息成功！"<<endl;
        }
    }
    return 0;
}

DWORD ExecFileProc(COMMAND command, SOCKET client) {
    COMMAND cmd;
    memset((char*)&cmd, 0, sizeof(cmd));
    cmd.ID = ExecFile;

    if (ShellExecute(NULL, "open", (char*)command.lparam, NULL, NULL, SW_MAXIMIZE) <= (HINSTANCE)32) {//打开执行文件
        strcpy_s((char*)cmd.lparam, 100, "文件执行失败！");
        send(client, (char*)&cmd, sizeof(cmd), 0);
    }
    else {
        strcpy_s((char*)cmd.lparam, 100, "文件执行成功！");
        send(client, (char*)&cmd, sizeof(cmd), 0);
    }

    return 0;
}

DWORD GetFileProc(COMMAND command, SOCKET client) {
    cout << "接收到客户端要下载文件的信息！\n";
    FILEINFO fi;
    COMMAND cmd;
    memset((char*)&fi, 0, sizeof(fi));
    memset((char*)&cmd, 0, sizeof(cmd));
    cmd.ID = GetFile;
    CFile file;
    int nChunkCount = 0;
    if (file.Open((char*)command.lparam, CFile::modeRead | CFile::typeBinary)) {
        int FileLen = file.GetLength();
        fi.FileLen = FileLen;
        strcpy_s((char*)fi.FileName,110, file.GetFileName());
        memcpy((char*)&cmd.lparam, (char*)&fi, sizeof(fi));
        send(client, (char*)&cmd, sizeof(cmd), 0);

        nChunkCount = FileLen / CHUNK_SIZE;//要传输多少次
        if (FileLen % CHUNK_SIZE != 0) {
            nChunkCount++;
        }
        cout << "jfkjskdlfjklsajk:" << nChunkCount;
        char* date = new char[CHUNK_SIZE];
        for (int i = 0; i < nChunkCount; i++) {
            int nLeft;//本次要传输多少
            if (i + 1 == nChunkCount) {
                nLeft = FileLen - CHUNK_SIZE * (nChunkCount - 1);
            }
            else
                nLeft = CHUNK_SIZE;
            int idx = 0;
            file.Read(date, CHUNK_SIZE);
            cout << "step:" << i << endl;
            while (nLeft > 0) {
                int ret = send(client, &date[idx], nLeft, 0);
                if (ret == SOCKET_ERROR) {
                    cout << "文件传输过程中发生错误！\n";

                }
                nLeft -= ret;
                idx += ret;
            }
        }
        file.Close();
        delete[] date;

    }
    return 0;
}

DWORD PutFileProc(COMMAND command, SOCKET client) {
    COMMAND cmd;
    memset((char*)&cmd, 0, sizeof(cmd));
    cmd.ID = PutFile;

    FILEINFO* fi = (FILEINFO*)command.lparam;
    int FileLen = fi->FileLen;
    CFile file;
    int nChunkCount = FileLen / CHUNK_SIZE;
    if (FileLen % CHUNK_SIZE != 0) {
        nChunkCount++;
    }

    if (file.Open(fi->FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) {
        char* date = new char[CHUNK_SIZE];
        for (int i = 0; i < nChunkCount; i++) {
            int nLeft;
            if (i + 1 == nChunkCount) {
                nLeft = FileLen - CHUNK_SIZE * (nChunkCount - 1);
            }
            else
                nLeft = CHUNK_SIZE;
            int idx = 0;
            while (nLeft > 0) {
                int ret = recv(client, &date[idx], nLeft, 0);
                if (ret == SOCKET_ERROR) {
                    cout << "接收上传文件错误！\n";
                    break;
                }
                nLeft -= ret;
                idx += ret;
            }
            file.Write(date,CHUNK_SIZE);
        }
        file.Close();
        delete[] date;
        cout << "接收上传文件成功！\n";

        strcpy_s((char*)cmd.lparam, 100, "上传文件成功！");
        send(client, (char*)&cmd, sizeof(cmd), 0);
    }
    else {
        strcpy_s((char*)cmd.lparam, 100, "上传文件失败！");
        send(client, (char*)&cmd, sizeof(cmd),0);
    }
    return 0;
}

DWORD FileInfoProc(COMMAND command, SOCKET client) {
    COMMAND cmd;
    FILEINFO fi;
    memset((char*)&cmd, 0, sizeof(cmd));
    memset((char*)&fi, 0, sizeof(fi));

    HANDLE hFile;
    WIN32_FIND_DATA WFD;//结构中包含文件信息
    memset((char*)&WFD, 0, sizeof(WFD));
    if ((hFile = FindFirstFile((char*)command.lparam, &WFD)) == INVALID_HANDLE_VALUE) {//通过FindFirstFile获得文件句柄
        fi.Error == TRUE;
        return 0;
    }

    DWORD FileLen;
    char stime[32];
    SHFILEINFO shfi;//结构中包含文件信息
    SYSTEMTIME systime;//结构中包含日期信息
    FILETIME localtime;//结构中包含文件时间信息

    memset(&shfi, 0, sizeof(shfi));

    SHGetFileInfo(WFD.cFileName, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);//得到指定文件的指定信息
    strcpy_s(fi.FileName, MAX_PATH, (char*)command.lparam);
    FileLen = (WFD.nFileSizeHigh * MAXDWORD) + WFD.nFileSizeLow;
    fi.FileLen = FileLen;

    FileTimeToLocalFileTime(&WFD.ftLastWriteTime, &localtime);//把utc标准时间转化成本地时间
    FileTimeToSystemTime(&localtime, &systime);//把本地时间转化成系统时间

    sprintf(stime, "%4d-%2d-%2d %2d:%2d:%2d", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);//把指定格式写入到字符串中
    strcpy_s(fi.Time, 100, stime);

    if (GetFileAttributes((char*)command.lparam) & FILE_ATTRIBUTE_HIDDEN) {//判断文件属性（只读隐藏）
        fi.IsDir = TRUE;
    }
    else {
        if (GetFileAttributes((char*)command.lparam) & FILE_ATTRIBUTE_READONLY) {
            fi.Error = TRUE;
        }
    }

    cmd.ID = GetFileInfo;
    memcpy((char*)cmd.lparam, (char*)&fi, sizeof(fi));


    send(client, (char*)&cmd, sizeof(cmd), 0);
    FindClose(hFile);

    return 0;
}

DWORD CreateDirProc(COMMAND command, SOCKET client) {
    COMMAND cmd;
    memset((char*)&cmd, 0, sizeof(cmd));
    cmd.ID = CreateDir;

    if (CreateDirectory((char*)command.lparam, NULL)) {
        strcpy_s((char*)cmd.lparam, 100, "创建目录成功！");
        send(client, (char*)&cmd, sizeof(cmd), 0);
    }
    else {
        strcpy_s((char*)&cmd.lparam, 100, "创建目录失败！");
        send(client, (char*)&cmd, sizeof(cmd), 0);
    }

    return 0;
}

DWORD DelDirProc(COMMAND command, SOCKET client) {
    COMMAND cmd;
    memset((char*)&cmd, 0, sizeof(cmd));
    cmd.ID = DelDir;

    if (DeleteDirectory((char*)command.lparam )== TRUE) {
        strcpy_s((char*)cmd.lparam, 100, "删除目录成功！");
        send(client, (char*)&cmd, sizeof(cmd), 0);

    }
    else {
        strcpy_s((char*)cmd.lparam, 100, "删除目录失败！");
        send(client, (char*)&cmd, sizeof(cmd), 0);
    }
    return 0;
}

BOOL DeleteDirectory(char* DirName) {
    CFileFind tempFile;
    char tempFileFind[200];
    sprintf(tempFileFind, "%s*.*", DirName);
    BOOL IsFinded;
    IsFinded = (BOOL)tempFile.FindFile(tempFileFind);
    while (IsFinded) {
        IsFinded = (BOOL)tempFile.FindNextFile();
        if (!tempFile.IsDots()) {
            char foundFileName[200];
            strcpy_s(foundFileName, MAX_PATH, tempFile.GetFileName().GetBuffer(200));
            if (tempFile.IsDirectory()) {
                char tempDir[200];
                sprintf(tempDir, "%s\\%s", DirName, foundFileName);
                DeleteDirectory(tempDir);

            }
            else {
                char tempFileName[200];
                sprintf(tempFileName, "%s\\%s", DirName, foundFileName);
                SetFileAttributes(tempFileName, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(tempFileName);
            }

        }
    }
    tempFile.Close();
    if (!RemoveDirectory(DirName)) {
        return FALSE;
    }
    return TRUE;

}

DWORD ShutDownProc(COMMAND command, SOCKET client) {


    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
