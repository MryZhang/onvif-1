#include "soapH.h"
#include "../gsoap/plugin/wsddapi.h"
// �������nsmap�ļ����������ͨ����
#include "wsdd.nsmap"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

//���õ�����ʽ����probe�����ͻ����������ȴ�һ���¼���Ӧ���¼��ο�event.c
//ֻ�ܽ���һ��event������ָ����ַprobe�������ֶ����IPC
static int probeUnicast(const char *endpoint, const char * types, const char *scopes,int timeout)
{
	struct soap *soap = soap_new1(SOAP_IO_UDP); // to invoke messages
	int ret;
	const char *id = soap_wsa_rand_uuid(soap);
	//���ó�ʱʱ��,>0��λΪ�� =0 �ò���ʱ <0��λΪ΢��
	soap->accept_timeout = soap->recv_timeout = soap->send_timeout = timeout;
	ret = soap_wsdd_Probe(soap,
			  SOAP_WSDD_MANAGED,//SOAP_WSDD_ADHOC,	// ad-hoc mode
			  SOAP_WSDD_TO_TS,	// to a TS
			  endpoint, // address of TS; "soap.udp://239.255.255.250:3702"
			  id,	// message ID
			  NULL, // ReplyTo,��ʾ��Ӧ��message ID,��Ϊ�������ط�������û�У���NULL
			  types, //types,��Ѱ���豸����"dn:NetworkVideoTransmitter tds:Device"
			  scopes,    //scopes,ָ��������Χ������ NULL
			  NULL);   //match by��ƥ��������� NULL
	if(ret!=SOAP_OK)
	{
		soap_print_fault(soap, stderr);
		printf("soap_wsdd_Probe error,ret=%d\n",ret);
	}
	soap_end(soap);
	soap_free(soap);
	return ret;
}

//���öಥ��ʽ����probe���������ز������ȴ��¼���Ӧ����ȡ�¼���ӦҪ�������¶�Ӧ��sock�����soap_wsdd_listen�������¼��ο�event.c
//���Խ��ն��event������̽�������д��ڵ�IPC
static int probeMulticast(const char *endpoint, const char * types, const char *scopes,int timeout)
{
	struct soap *serv = soap_new1(SOAP_IO_UDP); // to invoke messages
	int ret;
	const char *id = soap_wsa_rand_uuid(serv);
	
	ret = soap_wsdd_Probe(serv,
			  SOAP_WSDD_ADHOC,//SOAP_WSDD_ADHOC,	// ad-hoc mode
			  SOAP_WSDD_TO_TS,	// to a TS
			  endpoint, // address of TS; "soap.udp://239.255.255.250:3702"
			  id,	// message ID
			  NULL, // ReplyTo,��ʾ��Ӧ��message ID,��Ϊ�������ط�������û�У���NULL
			  types, //types,��Ѱ���豸����"dn:NetworkVideoTransmitter tds:Device"
			  scopes,    //scopes,ָ��������Χ������ NULL
			  NULL);   //match by��ƥ��������� NULL

	if(ret!=SOAP_OK)
	{
		soap_print_fault(serv, stderr);
		printf("soap_wsdd_Probe error,ret=%d\n",ret);
		goto ERR0;
	}	
	if (!soap_valid_socket(serv->socket))
	{ 
		soap_print_fault(serv, stderr);
		printf("sock is error\n");
  		ret = -1;
		goto ERR0;
	}
	serv->master = serv->socket;//����ָ���������޷�����
	ret = soap_wsdd_listen(serv,timeout);
	
	if(ret!=SOAP_OK)
	{
		soap_print_fault(serv, stderr);
		printf("soap_wsdd_listen error,ret=%d\n",ret);
		goto ERR0;
	}
ERR0:
	soap_end(serv);
	soap_free(serv);
	return ret;
}

//����ָ���˿ڣ��¼��ο�event.c
//���ڼ���IPC���߻����ߡ�����Ӧ�ͻ�̽��
//timeout���ó�ʱʱ��,>0��λΪ�� =0 �ò���ʱ <0��λΪ΢��
static int listenPort(int port,int timeout)
{
	struct soap *serv = soap_new1(SOAP_IO_UDP); // to invoke messages
	int ret;
	//�󶨶˿ں�
	if (!soap_valid_socket(soap_bind(serv,NULL,port, 100)))
	{ 
		soap_print_fault(serv, stderr);
		printf("soap_bind error\n");
  		ret = -1;
		goto ERR0;
	}
	//����ಥ��ַ
	struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(serv->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
		printf("add multiaddr is error\n");
    	goto ERR0;
    }
	//����
	ret = soap_wsdd_listen(serv,timeout);
	if(ret!=SOAP_OK)
	{
		soap_print_fault(serv, stderr);
		printf("soap_wsdd_listen error,ret=%d\n",ret);
		goto ERR0;
	}
	printf("soap_wsdd_listen return\n");
ERR0:
	soap_end(serv);
	soap_free(serv);
	return ret;
}
 
int main()
{
	int ret;
	char *endpoint = "soap.udp://239.255.255.250:3702";
	char *types = NULL;
	char *scopes = NULL;
	
	while(1)
	{
		ret = 0;
	//	listenPort(3702,0);
#if 0	//unicast test
		endpoint = "soap.udp://192.168.110.71:3702";
		types = "dn:NetworkVideoTransmitter";
		ret = probeUnicast(endpoint,types,scopes,5);
		types = "tds:Device";
		ret |= probeUnicast(endpoint,types,scopes,5);
#else 	//multicast test
		char *endpoint = "soap.udp://192.168.16.128:3702";
		ret = probeMulticast(endpoint,types,scopes,5);
#endif
		if(ret)
			printf("probe failed\n");
		else
			printf("probe successful\n");
		sleep(5);
	}
	return 0;
} 
