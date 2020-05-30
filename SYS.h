#pragma once

#define CHUNK_SIZE (32*1024)
#define GetDriver 0x01
#define GetDirInfo 0x02
#define ExecFile 0x03
#define GetFile 0x04
#define PutFile 0x05
#define GetFileInfo 0x06
#define CreateDir 0x07
#define DelDir 0x08
#define ShutDown 0x09