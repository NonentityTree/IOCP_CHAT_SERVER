// echo_server_iocp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <vector>
#include <string>

#define PORT 9000
#define BUFSIZE 512
#define MAX_CLIENT 256
struct SOCKETINFO
{
	OVERLAPPED overlapped;
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
	char* addr;
	WSABUF wsabuf;
};



//�۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg);
void SendMsg(char* msg, DWORD len, SOCKETINFO* sock);
void deleteCLNT(SOCKET sock);
SOCKET clientArray[MAX_CLIENT];


int clientCnt = 0;
char tagName[BUFSIZE] = "";

int main(int argc, char*argv[])
{
	int retval;
	HANDLE hThread;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 1;
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	for (int i = 0; i < si.dwNumberOfProcessors * 2; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}

	//socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) return -1;

	//bind()
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);


	retval = bind(listen_sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR) return -1;

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) return -1;

	//������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientAddr;
	int addrlen;
	DWORD recvBytes, flags;

	while (1)
	{
		//accept();
		addrlen = sizeof(clientAddr);
		client_sock = accept(listen_sock, (sockaddr*)&clientAddr, &addrlen);
		if (client_sock == INVALID_SOCKET) return -1;

		printf("[IOCP ����] Ŭ���̾�Ʈ ���� IP : %s, ��Ʈ ��ȣ : %d \n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		sprintf_s(tagName, "%s:%d", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		//���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		//���� ���� ����ü �Ҵ�
		SOCKETINFO *ptr = new SOCKETINFO;
		if (ptr == NULL) break;

		//���� ���� ����ü �ʱ�ȭ
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recvbytes = ptr->sendbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;
		ptr->addr = inet_ntoa(clientAddr.sin_addr);
		clientArray[++clientCnt] = client_sock;



		//�񵿱� ����� ���� (WSARecvȣ��)
		flags = 0;
		retval = WSARecv(client_sock, &ptr->wsabuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
				return -1;
			continue;
		}

	}

	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;

	//�������Լ� ���ڷ� ���޵� ����� �Ϸ� ��Ʈ �ڵ� �� ����
	HANDLE hcp = (HANDLE)arg;

	HANDLE hComport = (HANDLE)arg;
	SOCKET sock;
	DWORD bytesTrans;
	SOCKETINFO* sInfo;
	DWORD flags = 0;

	while (1)
	{
		retval = GetQueuedCompletionStatus(hcp, &bytesTrans, (LPDWORD)&sock, (LPOVERLAPPED*)&sInfo, INFINITE);
		sock = sInfo->sock;

		puts("messagge received!");

		if (bytesTrans == 0)
		{
			deleteCLNT(sock);
			closesocket(sock);
			continue;
		}


		SendMsg(sInfo->buf, bytesTrans, sInfo);
		ZeroMemory(&(sInfo->overlapped), sizeof(sInfo->overlapped));
		WSARecv(sock, &(sInfo->wsabuf), 1, NULL, &flags, &(sInfo->overlapped), NULL);
	}
	return 0;

}

void SendMsg(char* msg, DWORD len, SOCKETINFO* sock)
{

	for (int i = 1; i <= clientCnt; i++)
	{
		if (clientArray[i] != sock->sock)
		{
			send(clientArray[i], msg, len, 0);
		}

	}
}

void deleteCLNT(SOCKET sock)
{
	for (int i = 0; i < clientCnt; i++)
	{
		if (sock == clientArray[i])
		{
			while (i < clientCnt - 1)
			{
				clientArray[i] = clientArray[i + 1];
				i++;
			}
			clientCnt--;
			break;
		}
	}
}

