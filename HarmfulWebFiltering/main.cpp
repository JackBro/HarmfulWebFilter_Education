
//�� �ҽ��� �������� �������� �ִ��� ���� ������ �� �ֵ��� ���ʿ��� �κ��� �����Ͽ����ϴ�.

#define _CRT_SECURE_NO_WARNINGS

// �߿����
// ȣ��Ʈ������ ��Ʈ��ũ���� �����͸� �д� ����� �ٸ���.
// ȣ��Ʈ������ ��Ʋ��������� ������ ��Ʈ��ũ������ �򿣵������ �����͸� �д´�.


#include <winsock2.h>		//socket�� ���� �Լ��� ����ü�� ������ֱ� ����
#include <stdio.h>			// ������� ���� ���
#include <string.h>			//���ڿ� �Լ����� ����ϱ� ���� ���
#include <windivert.h>		//�����̹�Ʈ�� ����ϱ� ���� ���

#define MAXBUF  0xFFFF			
#define STARTHTTPOFFSET 54		//HTTPOFFSET�� ���� ������(ipv4����)
/*
* Pre-fabricated packets.
*/

typedef struct iptcp_hdr		//IP�� TCP ����� ����ü ������ ���콺 ������ ���Ƿ� �̵� ������ �ڼ��ϰ� ���� �� �� �� ����
{
	WINDIVERT_IPHDR ip;			//IP����� ����ü
	WINDIVERT_TCPHDR tcp;		//TCP����� ����ü
} TCPPACKET, *PTCPPACKET;

typedef struct ip6tcp_hdr		// ������ 128��Ʈ������ ��� ����ü
{
	WINDIVERT_IPV6HDR ipv6;		//ipv6�� ����ü, ũ�Ա� 32��Ʈ �� ���� ���� ũ�Ⱑ Ŀ��
	WINDIVERT_TCPHDR tcp;		//TCP�� 32��Ʈ�� ���� ����
} TCPV6PACKET, *PTCPV6PACKET;

typedef struct ipicmp_hdr		// icmp ���, ���ͳ� ȯ�濡�� ������ ���� ó���� �����ϴ� �뵵
{
	WINDIVERT_IPHDR ip;			//IP����� ����ü
	WINDIVERT_ICMPHDR icmp;		//ICMP����� ����ü
	UINT8 data[];				// icmp�����ʹ� ��Ʈ��ũ�� �ʼ� �������� ����
} ICMPPACKET, *PICMPPACKET;

typedef struct ipicmp6_hdr		//���� ����, �ֱ� ������ ���� ���������� 32��Ʈ���� 128��Ʈ�� Ȯ���ϸ� 
{								//�ٸ� ���������� ũ�⵵ Ȯ�� ��.
	WINDIVERT_IPV6HDR ipv6;
	WINDIVERT_ICMPV6HDR icmpv6;
	UINT8 data[];
} ICMPV6PACKET, *PICMPV6PACKET;


/*
* Prototypes.
*/

void mystrcpy(unsigned char *dest, unsigned char *src)		//strcpy�� ���� �Լ��̳� ���ڸ� unsigned char�� �Ѱ��ֱ� ���� ����
{															//src�� �ִ� ���ڿ��� dest�� ������
	int index = 0;											//�̰� ���� ����Ŵ� �̸��̳� ������ ���� �ʼ�
	// ������ NULL �̰ų� ����� NULL �̸� ����

	if (!src || !dest) exit(1);
	while ((*(src + index) != 13)){
		*(dest + index) = *(src + index);
		index++;

	}
	*(dest + index) = '\n';
	*(dest + index) = '\0';
}

/*
* modify strstr function.
*/
char *findStr(unsigned char *str1, char *str2)			// strstr�Լ��� ���� ������ ���ڰ� unsigned char�� ������ֱ����� ����.
{														// str1�ȿ� �ִ� �Լ� �� str2�� ���ԵǾ��ִ��� ã���ִ� �Լ�
	char *cp = (char *)str1;							// �̰� ���� ����Ŵ� �̸��̳� ������ ���� �ʼ�
	char *s1, *s2;

	if (!*str2) return (char *)str1;

	while (*cp)
	{
		s1 = cp;
		s2 = (char *)str2;

		while (*s1 && *s2 && !(*s1 - *s2)) s1++, s2++;
		if (!*s2) return cp;
		cp++;
	}
}
//////////////////////////////////////////////////////////////////////
//�Ʒ����� ������ �Լ����� �� ���� �ڵ忡 ����ִ� �Լ�����
//////////////////////////////////////////////////////////////////////


void PacketIpInit(PWINDIVERT_IPHDR packet)		//��Ŷ�� IP����� �ʱ�ȭ �����ֱ� ���� �Լ�
{												//���� �������� �ִ¸� �ֿ� 4�� ����,����,id,ttl�� �ʱ�ȭ�����ش�
	memset(packet, 0, sizeof(WINDIVERT_IPHDR));
	packet->Version = 4;						//IPv4�̱� ������ 4�� �־���
	packet->HdrLength = sizeof(WINDIVERT_IPHDR) / sizeof(UINT32);
	packet->Id = ntohs(0xDEAD);					//ntohs�򿣵�ȿ��� ��Ʋ��������� ��ȯ�ϴ��Լ�, ID�� ��Ŷ�� ���� ��ȣ
	packet->TTL = 64;							//����� 1���� ��ĥ ������ TTL���� 1���� ����
}

/*
* Initialize a TCPPACKET.
*/
void PacketIpTcpInit(PTCPPACKET packet)			//��Ŷ�� TCP ����� �ʱ�ȭ �����ֱ� ���� �Լ�
{
	memset(packet, 0, sizeof(TCPPACKET));		//packet �޸𸮸� 0���� �ʱ�ȭ		
	PacketIpInit(&packet->ip);					//ip�ʱ�ȭ �Լ�
	packet->ip.Length = htons(sizeof(TCPPACKET));		//��Ʋ����ȿ��� �򿣵������ �ٲ۴�. +��Ʋ����Ȱ� �򿣵���� ã�ƺ��� �𸣴°� �����
	packet->ip.Protocol = IPPROTO_TCP;					//�������ݿ��� TCP ���������� �־��ش� TCP�� 6 UDP�� 17
	packet->tcp.HdrLength = sizeof(WINDIVERT_TCPHDR) / sizeof(UINT32);		//�Ϲ������� TCP ����� ũ��� 20�̴�
}

/*
* Initialize an ICMPPACKET.
*/
void PacketIpIcmpInit(PICMPPACKET packet)		//ip�� icmp�� �ʱ�ȭ�ϱ� ���� �Լ�
{
	memset(packet, 0, sizeof(ICMPPACKET));
	PacketIpInit(&packet->ip);
	packet->ip.Protocol = IPPROTO_ICMP;
}

/*
* Initialize a PACKETV6.
*/
void PacketIpv6Init(PWINDIVERT_IPV6HDR packet)		//ipv6 �ʱ�ȭ�ϴ� �Լ�
{
	memset(packet, 0, sizeof(WINDIVERT_IPV6HDR));
	packet->Version = 6;
	packet->HopLimit = 64;
}

/*
* Initialize a TCPV6PACKET.
*/
void PacketIpv6TcpInit(PTCPV6PACKET packet)		//ipv6,tcp ����� �ʱ�ȭ�ϴ� �Լ�
{
	memset(packet, 0, sizeof(TCPV6PACKET));
	PacketIpv6Init(&packet->ipv6);
	packet->ipv6.Length = htons(sizeof(WINDIVERT_TCPHDR));
	packet->ipv6.NextHdr = IPPROTO_TCP;
	packet->tcp.HdrLength = sizeof(WINDIVERT_TCPHDR) / sizeof(UINT32);
}

/*
* Initialize an ICMP PACKET.
*/
void PacketIpv6Icmpv6Init(PICMPV6PACKET packet)		//icmpv6 ��Ŷ�� �ʱ�ȭ�ϴ� �Լ�
{
	memset(packet, 0, sizeof(ICMPV6PACKET));
	PacketIpv6Init(&packet->ipv6);
	packet->ipv6.NextHdr = IPPROTO_ICMPV6;
}

/*
* Entry.
*/
int __cdecl main(int argc, char **argv)
{
	HANDLE handle;							
	UINT i;
	INT16 priority = 0;
	unsigned char packet[MAXBUF];
	unsigned char site[100];
	char buf[1024] = { 0, };
	FILE *f_log_txt;
	FILE *f_malsite_txt;
	UINT packet_len;
	WINDIVERT_ADDRESS recv_addr, send_addr;
	PWINDIVERT_IPHDR ip_header;
	PWINDIVERT_IPV6HDR ipv6_header;
	PWINDIVERT_ICMPHDR icmp_header;
	PWINDIVERT_ICMPV6HDR icmpv6_header;
	PWINDIVERT_TCPHDR tcp_header;
	PWINDIVERT_UDPHDR udp_header;
	UINT payload_len;
	TCPPACKET reset0;
	PTCPPACKET reset = &reset0;
	unsigned char *tcppacket;
	bool malsite_check = false;

	// Initialize all packets.
	PacketIpTcpInit(reset);		//reset�̶�� ������ �־� ��Ŷ�� �ʱ�ȭ��Ų��.
								//���� mal_site�� ������ ��� return���� ��Ŷ�� ������ ������
								//�ʱ�ȭ�� ��Ŷ�� ������ ������ ��� �ȴ�.
	reset->tcp.Rst = 1;			//TCP �÷��׿� ���� �����غ��� http://blog.naver.com/arottinghalo/40170289964
	reset->tcp.Ack = 1;

	// Divert traffic matching the filter:		
	handle = WinDivertOpen("outbound and tcp.PayloadLength > 0 and tcp.DstPort == 80", WINDIVERT_LAYER_NETWORK, 0, 0);
	//�� �Լ� ���� �߿��ϴ�. outbound and tcp.PayloadLength > 0 and tcp.DstPort == 80 �� ������ 
	//������ ��Ŷ�߿� tcp�� ���̷ε� ���̰� 0 �̻��̰� tcp port�� 80�� ���� ��ڴٴ� �ǹ��̴�.
	//�� http�� ������ ���� ��´�.		true�� ������ ��� ��Ŷ�� ������ ���Ϳ� �� ������ windivert���� Ȩ�� �����ؼ� ����ϸ� �ȴ�.
	
	if (handle == INVALID_HANDLE_VALUE)		// WinDivertOpen�� ����� ���ȴ��� Ȯ���ϴ� ���̴�.
	{										// Ȯ�� ��� ����� ���� ���� ������ �Լ� GetLastError()���� ������ ���� ��ȣ�� ����Ѵ�.
		if (GetLastError() == ERROR_INVALID_PARAMETER)		//��ȣ�� ã�ƺ��� � ������ ������ ������ Ȯ���� �� �ִ�.
		{
			fprintf(stderr, "error: filter syntax error\n");
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "error: failed to open the WinDivert device (%d)\n",
			GetLastError());
		exit(EXIT_FAILURE);
	}

	// Main loop:
	while (TRUE)
	{
		malsite_check = false;							//
		f_log_txt = fopen("log.txt", "a");				//log.txt������ ���� ���� ���� ���� �����. r�б� w���� a�а� ������ ���� ����
		f_malsite_txt = fopen("mal_site.txt", "r");		//mal_site.txt�� ����.	�׽�Ʈ�� �� �޸��� �ȿ� �ִ� ����Ʈ�� �׽�Ʈ ����		
		// Read a matching packet.
		if (!WinDivertRecv(handle, packet, sizeof(packet), &recv_addr, &packet_len))		//���⼭ ��Ŷ�� ��� �Լ�
		{																					//��Ŷ�� �� �� ���� ���� �ִ´�.
			fprintf(stderr, "warning: failed to read packet\n");
			continue;
		}

		// Print info about the matching packet.
		WinDivertHelperParsePacket(packet, packet_len, &ip_header,			//WinDivertHelperParsePacket�� �Լ��� packet�� �ִ� ��������
			&ipv6_header, &icmp_header, &icmpv6_header, &tcp_header,		//�� �̾Ƽ� ����ü�� �־��ִ� �Լ���.
			&udp_header, NULL, &payload_len);								//������ �������� �� �̾ƴٰ� ����ü�� �־��ش�.
		if (ip_header == NULL && ipv6_header == NULL) continue;				//ip,ipv6����� NULL�� ��� �ٽ� while�� ���ư���.
		tcppacket = (unsigned char*)malloc(packet_len);						//��Ŷ�� ���̸�ŭ malloc(���� �޸��Ҵ�)���� tcppacket�� �޸𸮸� �Ҵ����ش�
		memcpy(tcppacket, packet, packet_len);								//packet�� �����͵��� tcppacket�� ī�����ش�.

		//get host
		for (int i = STARTHTTPOFFSET; i < packet_len; i++)	//HTTP���� ��Ŷ�� ȣ��Ʈ�� �ּҸ� �˱� ���� �Լ���.
		{
			if (tcppacket[i] == 'H' && tcppacket[i + 1] == 'o' && tcppacket[i + 2] == 's' && tcppacket[i + 3] == 't')	//Wireshark���� ��Ŷ�� ���� �˰����� Host:nate.com �̷������� �����Ͱ� ����ִ�.
			{																											//�׷��� ��Ŷ���� Host�� ����ִ� �κ��� ã�ƺ���
				mystrcpy(site, tcppacket + i + 5);			//��Ŷ���� ȣ��Ʈ�� �ּҸ� ã������ Host:������ �ּҵ��� site�� �־��ش�.
				break;
			}
		}


		//�Ǽ� ����Ʈ�� ħ���� ���� ����
		/////////////////////////////////////////////////////////////////////////////////////////
		while (!feof(f_malsite_txt))		//mal_site.txt�� NULL�� �� ������ �д´�. ������ ���� �ϰڴٴ� �Ҹ���. 
		{
			//read(f_malsite_txt, buf, 1024);
			fgets(buf, 1024, f_malsite_txt);		//�޸����� �� �پ� �о buf�� ��´�.
			for (int i = 0; i < sizeof(buf); i++)
			{
				if (buf[i] == 10)					//�޸��� ���� \n�� ���� site�ּҿ� �񱳰� �ȵǴ� \n�� 0���� ����������. 
				{
					buf[i] = 0;
					break;
				}
			}
			if (findStr(site, buf))					//������ ����Ʈ�� mal_site�� �����ִ� ����Ʈ������ üũ�Ѵ�.
			{										//������ ����Ʈ�� �´ٸ� ������ �̾ƿ´�
				UINT8 *src_addr = (UINT8 *)&ip_header->SrcAddr;		//src ������ �ּ� ����
				UINT8 *dst_addr = (UINT8 *)&ip_header->DstAddr;		//dest ������ ����
				printf("BLCOK! site : %s ip.SrcAddr=%u.%u.%u.%u ip.DstAddr=%u.%u.%u.%u\n", buf,
					src_addr[0], src_addr[1], src_addr[2], src_addr[3],	//srd�� dest�� �����Ǹ� �ֿܼ� ����Ͽ� �ش�.
					dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3]);
				fprintf(f_log_txt, "BLCOK! site : %s ip.SrcAddr=%u.%u.%u.%u ip.DstAddr=%u.%u.%u.%u\n", buf,
					src_addr[0], src_addr[1], src_addr[2], src_addr[3],		//log.txt�� fprintf �Լ��� �̿��Ͽ� �α׸� �����.
					dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3]);
				fclose(f_log_txt);
				malsite_check = true;
				break;
			}
		}
		if (malsite_check == true)
		{	//������ ����Ʈ�� ���ػ���Ʈ�� ��� ��Ŷ�� �������� �ʰ� ������. or �ʱ�ȭ�� ��Ŷ�� ������ ������.
			continue;
			//WinDivertSend(handle, (PVOID)reset, sizeof(TCPPACKET), &send_addr, NULL);
		}
		else
		{	//������ ����Ʈ�� ���ػ���Ʈ�� �ƴ� ��� ���� ��Ŷ�� �ٽ� send���ش�.
			WinDivertSend(handle, (PVOID)packet, sizeof(packet), &send_addr, NULL);
		}	
		//send�� ������ �߻��ؼ� ���Ǹ� �غ���
		//�Ϻ� Windows 10���� send�� �߻����� �ʴ� ������ �ִٰ� �����ڰ� ������ �Դ�.
		//send�� �ȵǵ� ����������.
		putchar('\n');
	}
	return 1;
}
