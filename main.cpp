// Example command line:
// local:192.168.0.12 remote:192.168.0.15:80
// Each command line entry will be sub-string matched against each entry, if all entries match then it is a match and the entry will be closed
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <Iphlpapi.h>
#include <WinSock.h>
#include <string>

#pragma comment(lib , "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

int main(int argc , char **argv)
{
	// No need to use AllocateAndGetTcpExTableFromStack

	DWORD size = 0;
	DWORD ret = GetExtendedTcpTable(NULL , &size , TRUE , AF_INET , TCP_TABLE_OWNER_PID_ALL , 0);

	PMIB_TCPTABLE_OWNER_PID table = (PMIB_TCPTABLE_OWNER_PID) malloc(size);
	ret = GetExtendedTcpTable((PVOID)table , &size , TRUE , AF_INET , TCP_TABLE_OWNER_PID_ALL , 0);

	bool displayErrorElevate = false;

	printf("Entries:%d size:%d\n" , table->dwNumEntries , size);

	size_t i = 0;
	for (i = 0 ; i < table->dwNumEntries ; i++)
	{
		char *state = "";
		switch(table->table[i].dwState)
		{
		case MIB_TCP_STATE_CLOSED:
			state = "CLOSED";
			break;

		case MIB_TCP_STATE_LISTEN:
			state = "LISTEN";
			break;

		case MIB_TCP_STATE_SYN_SENT:
			state = "SYN_SENT";
			break;

		case MIB_TCP_STATE_SYN_RCVD:
			state = "SYN_RCVD";
			break;

		case MIB_TCP_STATE_ESTAB:
			state = "ESTABLISHED";
			break;

		case MIB_TCP_STATE_FIN_WAIT1:
			state = "FIN_WAIT1";
			break;

		case MIB_TCP_STATE_FIN_WAIT2:
			state = "FIN_WAIT2";
			break;

		case MIB_TCP_STATE_CLOSE_WAIT:
			state = "CLOSE_WAIT";
			break;

		case MIB_TCP_STATE_CLOSING:
			state = "CLOSING";
			break;

		case MIB_TCP_STATE_LAST_ACK:
			state = "LAST_ACK";
			break;

		case MIB_TCP_STATE_TIME_WAIT:
			state = "TIME_WAIT";
			break;

		case MIB_TCP_STATE_DELETE_TCB:
			state = "DELETE_TCB";
			break;

		default:
			state = "<undefined>";
			break;
		}

		/*
		toMatch will contain a string like:
			Index:34 State:ESTABLISHED:5 pid:2120
			local:192.168.0.12:5218
			remote:192.168.0.15:80
		*/
		std::string toMatch;
		char tempString[1024];

		sprintf(tempString , "Index:%d State:%s:%d pid:%d\n" , i , state , table->table[i].dwState , table->table[i].dwOwningPid);
		toMatch += tempString;

		struct in_addr laddr;
		laddr.S_un.S_addr = table->table[i].dwLocalAddr;
		char *addr = inet_ntoa(laddr);
		sprintf(tempString , "local:%s:%d\n" , addr , ntohs((u_short) table->table[i].dwLocalPort));
		toMatch += tempString;

		laddr.S_un.S_addr = table->table[i].dwRemoteAddr;
		addr = inet_ntoa(laddr);
		sprintf(tempString , "remote:%s:%d\n" , addr , ntohs((u_short) table->table[i].dwRemotePort));
		toMatch += tempString;

		bool matching = false;
		int j;
		for (j = 1 ; j < argc ; j++)
		{
			if (strstr(toMatch.c_str() , argv[j]))
			{
				matching = true;
			}
			else
			{
				matching = false;
				break;
			}
		}

		if (matching)
		{
			printf("**Matched**\n");
		}

		// If it is matched, or there are no options then display the list
		if (matching || argc == 1)
		{
			printf(toMatch.c_str());
		}

		if (matching)
		{
			MIB_TCPROW toDisconnect;
			toDisconnect.State = MIB_TCP_STATE_DELETE_TCB;
			toDisconnect.dwLocalAddr = table->table[i].dwLocalAddr;
			toDisconnect.dwLocalPort = table->table[i].dwLocalPort;
			toDisconnect.dwRemoteAddr = table->table[i].dwRemoteAddr;
			toDisconnect.dwRemotePort = table->table[i].dwRemotePort;
			ret = SetTcpEntry(&toDisconnect);
			if (0 != ret)
			{
				printf("Error:%d\n" , ret);
				if (317 == ret)
				{
					displayErrorElevate = true;
				}
			}
		}
	}

	if (displayErrorElevate)
	{
		printf("Run as elevated admin to close connections\n");
	}

	return 0;
}
