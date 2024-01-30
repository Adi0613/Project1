#include <winsock2.h>
#include <stdio.h>
#include <assert.h>
#include "SocketHelper.h"
#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
#endif

int main(int argc, char* argv[])
{
    SOCKET sock;
    sockaddr_in sin;
    DWORD packets;
    bool hijack = false;
    bool nohijack = false;

    if (argc < 2 || argc > 3)
    {
        printf("Usage is %s [address to bind]\n", argv[0]);
        printf("Options are:\n\t-hijack\n\t-nohijack\n");
        return -1;
    }

    if (argc == 3)
    {
        if (strcmp("-hijack", argv[2]) == 0)
        {
            hijack = true;
        }
        else if (strcmp("-nohijack", argv[2]) == 0)
        {
            nohijack = true;
        }
        else
        {
            printf("Unrecognized argument %s\n", argv[2]);
            return -1;
        }
    }

    if (!InitWinsock())
        return -1;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock == INVALID_SOCKET)
    {
        printf("Cannot create socket -  err = %d\n", WSAGetLastError());
        WSACleanup();  // Don't forget to clean up Winsock
        return -1;
    }

    if (!InitSockAddr(&sin, argv[1], 8391))
    {
        printf("Can’t initialize sockaddr_in - doh!\n");
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    if (hijack)
    {
        BOOL val = TRUE;
        if (setsockopt(sock,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char*)&val,
            sizeof(val)) == 0)
        {
            printf("SO_REUSEADDR enabled -  Yo Ho Ho\n");
        }
        else
        {
            printf("Cannot set SO_REUSEADDR -  err = %d\n",
                WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return -1;
        }
    }
    else if (nohijack)
    {
        BOOL val = TRUE;
        if (setsockopt(sock,
            SOL_SOCKET,
            SO_EXCLUSIVEADDRUSE,
            (char*)&val,
            sizeof(val)) == 0)
        {
            printf("SO_EXCLUSIVEADDRUSE enabled\n");
            printf("No hijackers allowed!\n");
        }
        else
        {
            printf("Cannot set SO_EXCLUSIVEADDRUSE -  err = %d\n",
                WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return -1;
        }
    }

    if (bind(sock, (sockaddr*)&sin, sizeof(sockaddr_in)) == 0)
    {
        printf("Socket bound to %s\n", argv[1]);
    }
    else
    {
        if (hijack)
        {
            printf("Curses! Our evil warez are foiled!\n");
        }

        printf("Cannot bind socket -  err = %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    for (packets = 0; packets < 10; packets++)
    {
        char buf[512];
        sockaddr_in from;
        int fromlen = sizeof(sockaddr_in);

        if (recvfrom(sock, buf, 512, 0, (sockaddr*)&from, &fromlen) > 0)
        {
            printf("Message from %s at port %d:\n%s\n",
                inet_ntoa(from.sin_addr),
                ntohs(from.sin_port),
                buf);

            if (hijack)
            {
                sockaddr_in local;
                if (InitSockAddr(&local, "127.0.0.1", 8391))
                {
                    buf[sizeof(buf) - 1] = '\0';
                    strncpy(buf, "You are hacked!", sizeof(buf) - 1);
                    if (sendto(sock,
                        buf,
                        strlen(buf) + 1, 0,
                        (sockaddr*)&local,
                        sizeof(sockaddr_in)) < 1)
                    {
                        printf("Cannot send message to localhost - err = %d\n",
                            WSAGetLastError());
                    }
                }
            }
        }
        else
        {
            printf("Ghastly error %d\n", WSAGetLastError());
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
