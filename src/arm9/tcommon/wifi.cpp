#include "wifi.h"

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "../../common/fifo.h"
#include "common.h"
#include "parser/http.h"
#include "parser/ini_parser.h"

//Macro's
#define WIFI_PRINT(x, y...) (printf(x, ## y)) //vararg macro, woot

//Variables
WifiSettings wifiConfig;
int assocStatus;

void initWifi() {
	Wifi_InitDefault(false);
	Wifi_DisableWifi();
}

void startWifi(WifiSettings* config) {
	Wifi_EnableWifi();
	assocStatus = ASSOCSTATUS_DISCONNECTED;

	if (config) {
		if (config->dhcp) {
			Wifi_SetIP(0, 0, 0, 0, 0);
		} else {
			Wifi_SetIP(config->ip, config->gateway, config->subnetMask, config->dns1, config->dns2);
		}

		unsigned char temp[32];
		if (config->wepKeyType == ASCII) {
	    	memcpy(temp, config->wepKey, 16);
		} else {
			int n;
			for (n = 0; n < 16 && config->wepKey[n] != '\0'; n+=2) {
				temp[n>>1] = (chartohex(config->wepKey[n])<<4)|chartohex(config->wepKey[n+1]);
				if (config->wepKey[n+1] == '\0') {
					n += 2;
					break;
				}
			}
		}
		
		Wifi_ConnectAP(&config->ap, config->wepMode, 0, temp);
	} else {
		Wifi_AutoConnect();
	}
}
void stopWifi() {
    Wifi_DisableWifi();
	assocStatus = ASSOCSTATUS_DISCONNECTED;
}

u32 connectWifi() {
    int lastAssocStatus = assocStatus;
    assocStatus = Wifi_AssocStatus();

    if (lastAssocStatus != assocStatus) {
        switch (assocStatus) {
            case ASSOCSTATUS_DISCONNECTED:
                WIFI_PRINT("Wifi: Disconnected.\n");
                break;
            case ASSOCSTATUS_SEARCHING:
                WIFI_PRINT("Wifi: Searching for an AP...\n");
                break;
            case ASSOCSTATUS_AUTHENTICATING:
                WIFI_PRINT("Wifi: Authenticating...\n");
                break;
            case ASSOCSTATUS_ASSOCIATING:
                WIFI_PRINT("Wifi: Connecting...\n");
                break;
            case ASSOCSTATUS_ACQUIRINGDHCP:
                WIFI_PRINT("Wifi: Getting IP from DHCP...\n");
                break;
            case ASSOCSTATUS_ASSOCIATED:
                WIFI_PRINT("Wifi: Connected.\n");
                break;
            case ASSOCSTATUS_CANNOTCONNECT:
                WIFI_PRINT("Wifi: Connection error.\n");
                break;
            default:
                WIFI_PRINT("Wifi: Unknown status (%d)\n", assocStatus);
        }
    }
    return assocStatus;
}

bool downloadFile(const char* url, const char* outputFile) {
    unlink(outputFile);

    char baseUrl[128];
    char relPath[256];

    if (strncmp("http://", url, 7) == 0) {
    	url += 7;
    }

    char* temp = strchr(url, '/');
    if (temp) {
        strncpy(baseUrl, url, temp-url);
        baseUrl[temp-url] = '\0';
        strcpy(relPath, temp);
    } else {
        strcpy(baseUrl, url);
        strcpy(relPath, "/");
    }

    TCPConnection con;
    if (con.Connect(baseUrl) < 0) {
        WIFI_PRINT("Can't connect to %s\n", baseUrl);
        return false;
    } else {
        //WIFI_PRINT("Connected to %s\n", baseUrl);

        const char* request = "GET %s HTTP/1.1\r\n"
                              "Host: %s\r\n"
                              "\r\n";
        char buffer[1024];
        sprintf(buffer, request, relPath, baseUrl);

        //Send HTTP request
        con.Send(buffer, strlen(buffer));

        //Wait for answer
        HttpHeader header;
        HttpHeaderEntry* he;

        u32 readOffset = header.ReadFromTCP(buffer, 1024, &con);
        u32 totalRead = readOffset;
        u32 contentLength = 0;
        he = header.GetEntry("Content-Length");
        if (he) {
            contentLength = he->AsInt();
            //WIFI_PRINT("Content-Length: %d\n", contentLength);
        } else {
            con.Disconnect();
            return false;
        }

        WIFI_PRINT("Downloading: %s\n", url);

        FILE* tempFile = fopen(outputFile, "w");
        if (tempFile) {

        	u32 timeTaken = 0;
        	u32 dls = 0;
        	u32 ltr = 0;
			while (totalRead < contentLength) {
	            int r = con.Receive(buffer + readOffset, 1024);
				if (r > 0) {
					readOffset += r;
					totalRead += r;
				} else if (r == 0) {
					break;
				}

				u32 p = 10000*(totalRead>>10)/(contentLength>>10);
				WIFI_PRINT("\x1b[s");
		        WIFI_PRINT("Downloading: %3d.%02d%%  %3d.%02dKB/s", p/100, p%100,
		        		f32toint(dls>>10), f32toint((100*dls)>>10)%100);
				WIFI_PRINT("\x1b[u");

		        fwrite(buffer, 1, readOffset, tempFile);
	            readOffset = 0;

	            //Recalculate download speed
	            swiWaitForVBlank();
				timeTaken += floattof32(16.67f);
				if (timeTaken >= inttof32(500)) {
					s32 bytesPerSecond = divf32(inttof32(totalRead-ltr), timeTaken/1000);

					if (dls == 0) dls = bytesPerSecond;
					else dls = (3*dls + bytesPerSecond)>>2;

					ltr = totalRead;
					timeTaken = 0;
				}
			}
	        WIFI_PRINT("\x1b[KDownloading: 100.00%%\n");
            fwrite(buffer, 1, readOffset, tempFile);
            readOffset = 0;

            fclose(tempFile);
            con.Disconnect();
            return true;
        }
        WIFI_PRINT("Error writing to file %s\n", outputFile);
        con.Disconnect();
    }
    return false;
}

char* ipToString(char* out, u32 ip) {
	sprintf(out, "%d.%d.%d.%d", (ip>>24) & 0xFF, (ip>>16) & 0xFF, (ip>>8) & 0xFF, ip & 0xFF);
	return out;
}
u32 stringToIP(const char* ip) {
	char temp[16];
	strncpy(temp, ip, 15);
	temp[15] = '\0';

	char* str1 = strtok(temp, ".");
	if (!str1) return 0;
	char* str2 = strtok(NULL, ".");
	if (!str2) return 0;
	char* str3 = strtok(NULL, ".");
	if (!str3) return 0;
	char* str4 = strtok(NULL, ".");
	if (!str4) return 0;

	return ((atoi(str1)&0xFF)<<24)|((atoi(str2)&0xFF)<<16)|((atoi(str3)&0xFF)<<8)|(atoi(str4)&0xFF);
}

//=============================================================================

WifiSettings::WifiSettings() {
	Reset();
}
WifiSettings::~WifiSettings() {

}

void WifiSettings::Reset() {
	memset(&ap, 0, sizeof(Wifi_AccessPoint));

	strcpy(ap.ssid, "linksys");
	ap.ssid_len = strlen(ap.ssid);

	wepKeyType = HEX;
	wepMode = WEPMODE_NONE;
	dhcp = 1;
	ip = 0xC0A801BD;
	subnetMask = 0xFFFFFF00;
	gateway = 0xC0A80101;
	dns1 = 0xC0A80101;
	dns2 = 0x00000000;
}

bool WifiSettings::Load(const char* path) {
	IniFile iniFile;
	if (iniFile.Load(path)) {
        IniRecord* r;

        Reset();

        r = iniFile.GetRecord("ssid");
        if (r) {
        	strncpy(ap.ssid, r->AsString(), 33);
        	ap.ssid_len = strlen(ap.ssid);
        }
        r = iniFile.GetRecord("wepkey");
        if (r) strncpy((char*)wepKey, r->AsString(), 16);
        r = iniFile.GetRecord("wepkeytype");
        if (r) wepKeyType = (strcmp(r->AsString(), "hex") == 0 ? HEX : ASCII);
        r = iniFile.GetRecord("wepmode");
        if (r) wepMode = r->AsInt();
        r = iniFile.GetRecord("dhcp");
        if (r) dhcp = r->AsInt();
        r = iniFile.GetRecord("ip");
        if (r) ip = stringToIP(r->AsString());
        r = iniFile.GetRecord("subnetmask");
        if (r) subnetMask = stringToIP(r->AsString());
        r = iniFile.GetRecord("gateway");
        if (r) gateway = stringToIP(r->AsString());
        r = iniFile.GetRecord("dns1");
        if (r) dns1 = stringToIP(r->AsString());
        r = iniFile.GetRecord("dns2");
        if (r) dns2 = stringToIP(r->AsString());
	} else {
		return false;
	}

	return true;
}
void WifiSettings::Save(const char* path) {
	FILE* file = fopen(path, "w");
	if (!file) return;

	fprintf(file, "ssid=%s\n", ap.ssid);
	fprintf(file, "wepkey=%s\n", wepKey);
	fprintf(file, "wepkeytype=%s\n", wepKeyType == HEX ? "hex" : "ascii");
	fprintf(file, "wepmode=%d\n", wepMode);
	fprintf(file, "dhcp=%d\n", dhcp);

	char temp[32];
	fprintf(file, "ip=%s\n", ipToString(temp, ip));
	fprintf(file, "subnetmask=%s\n", ipToString(temp, subnetMask));
	fprintf(file, "gateway=%s\n", ipToString(temp, gateway));
	fprintf(file, "dns1=%s\n", ipToString(temp, dns1));
	fprintf(file, "dns2=%s\n", ipToString(temp, dns2));

	fclose(file);
}

//=============================================================================

TCPConnection::TCPConnection(int s) {
	sock = s;
    blocking = false;
}
TCPConnection::~TCPConnection() {
    Disconnect();
}

int TCPConnection::Connect(const char* url, u16 port) {
	Disconnect();

    hostent* hostent = gethostbyname(url);
    if (!hostent) {
        WIFI_PRINT("TCP Connect Error: gethostbyname returned: %d\n", errno);
        return 0;
    }

    u32 ip = *((u32*)(hostent->h_addr_list[0]));

    return Connect(ip, port);
}

int TCPConnection::Connect(u32 ip, u16 port) {
	Disconnect();

    sockaddr_in sain;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&sain, 0, sizeof(sain));
    sain.sin_addr.s_addr = htonl(ip);
    sain.sin_port = htons(port);

    if (connect(sock, (struct sockaddr*)&sain, sizeof(sain)) < 0) {
        WIFI_PRINT("TCP Connect Error: connect returned: %d\n", errno);
        Disconnect();
        return -1;
    }

    if (!blocking) {
    	SetBlocking(blocking);
    }

    return sock;
}

int TCPConnection::Server(u16 port, u8 maxConnections) {
    sockaddr_in sain;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&sain, 0, sizeof(sain));
    sain.sin_addr.s_addr = INADDR_ANY;
    sain.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&sain, sizeof(sain)) < 0) {
        WIFI_PRINT("TCP Server Error: bind returned: %d\n", errno);
        Disconnect();
        return 0;
    }
    if (listen(sock, maxConnections) < 0) {
        WIFI_PRINT("TCP Server Error: listen returned: %d\n", errno);
        Disconnect();
        return 0;
    }

    if (!blocking) {
    	SetBlocking(blocking);
    }

    return sock;
}

void TCPConnection::Disconnect() {
    if (sock >= 0) {
        closesocket(sock);
        sock = -1;
    }
}

void TCPConnection::Send(const void* rawdata, int dataL) {
    if (sock >= 0) {
    	const char* data = (const char*)rawdata;

        int offset = 0;
        int read;
        do {
            read = send(sock, data + offset, dataL - offset, 0);
            if (read > 0) offset += read;
        } while (read >= 0 && offset < dataL);
    }
}

int TCPConnection::Receive(void* out, int outL) {
    if (sock >= 0) {
        return recv(sock, out, outL, 0);
    }
    errno = 456;
    return -1;
}
int TCPConnection::ReceiveNow(void* out, int outL) {
	char* data = (char*)out;
	int offset = 0;
	int len = outL;

	while (len > 0) {
		int r = Receive(data+offset, len);
		if (r < 0) return r;

		offset += r;
		len -= r;
	}

	return offset;
}

void TCPConnection::SetBlocking(bool b) {
	blocking = b;

    int param = (b ? 0 : 1);
    ioctl(sock, FIONBIO, &param);
}

//=============================================================================

int udpCreateSocket(u16 port) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0){
        WIFI_PRINT("UDP Init Error: socket() failed: %d\n", errno);
		return -1;
	}

    sockaddr_in sain;
	memset(&sain, 0, sizeof(sain));
	sain.sin_family = AF_INET;
	sain.sin_port = htons(port);
	sain.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (sockaddr*)&sain, sizeof(sain)) < 0) {
        WIFI_PRINT("UDP Init Error: bind() failed: %d\n", errno);
        udpCloseSocket(sock);
		return -1;
	}

    int params[] = { 1 };
    if (ioctl(sock, FIONBIO, params) < 0) { //set non-blocking=1
        WIFI_PRINT("UDP Init Error: ioctl() returned: %d\n", errno);
        udpCloseSocket(sock);
        return -1;
    }

	return sock;
}

int udpSend(int sock, u32 destIP, u16 destPort, const void* data, int dataL) {
	//if (dataL > 512) WIFI_PRINT("Warning: UDP data send size is dangerously large\n");

    sockaddr_in sain;
	sain.sin_family = AF_INET;
	sain.sin_port = htons(destPort);
	sain.sin_addr.s_addr = htonl(destIP);

	int offset = 0;
	int len = dataL;
	while (len > 0) {
		int r = sendto(sock, ((const char*)data)+offset, len, 0, (sockaddr*)&sain, sizeof(sain));
		if (r < 0) return -1;

		offset += r;
		len -= r;
	}
	return offset;
}

//Returns the length of the received packet
int udpReceive(int sock, void* out, int outL, sockaddr_in* outAddr, int* outAddrL) {
    sockaddr_in sain;
    int i = sizeof(sain);

	if (!outAddr) outAddr = &sain;
	if (!outAddrL) outAddrL = &i;

	return recvfrom(sock, out, outL, 0, (sockaddr*)outAddr, outAddrL);
}

void udpCloseSocket(int sock) {
    if (sock >= 0) {
        closesocket(sock);
    }
}
