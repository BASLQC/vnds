#ifndef WIFI_H
#define WIFI_H

#include "common.h"
#include <dswifi9.h>

struct sockaddr_in;

enum WepKeyType {
	HEX,
	ASCII
};

class WifiSettings {
	private:

	public:
		Wifi_AccessPoint ap;
		unsigned char wepKey[16];
		WepKeyType wepKeyType;
		u8  wepMode;
		u8  dhcp; // 0=none, 1=get IP&dns, 2=get IP&not dns
		u32 ip;
		u32 subnetMask;
		u32 gateway;
		u32 dns1;
		u32 dns2;

		WifiSettings();
		virtual ~WifiSettings();

		void Reset();
		bool Load(const char* path);
		void Save(const char* path);
};

class TCPConnection {
    private:
        bool blocking;

    public:
        int sock;

        TCPConnection(int sock=-1);
        ~TCPConnection();

        int  Connect(const char* url, u16 port=80);
        int  Connect(u32 ip, u16 port);
        int  Server(u16 port, u8 maxConnections);
        void Disconnect();

        void Send(const void* data, int dataL);
        int  Receive(void* out, int outL);
        int  ReceiveNow(void* out, int outL);

        void SetBlocking(bool b);
};

extern WifiSettings wifiConfig;

void  initWifi(); //Call this at the start of the program to allow wifi to activate
void  startWifi(WifiSettings* config); //Turn on wifi and connect to default AP
u32   connectWifi(); //Wait for a connection
bool  downloadFile(const char* url, const char* outputFile);
void  stopWifi(); //Disconnect wifi and turn off hardware

char* ipToString(char* out, u32 ip);
u32   stringToIP(const char* ip);

int   udpCreateSocket(u16 port);
void  udpCloseSocket(int sock);
int   udpSend(int sock, u32 destIp, u16 destPort, const void* data, int dataL);
int   udpReceive(int sock, void* out, int outL, sockaddr_in* outAddr=NULL, int* outAddrL=NULL);

#endif
