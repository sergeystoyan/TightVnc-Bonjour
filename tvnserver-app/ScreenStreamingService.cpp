//********************************************************************************************
//Author: Sergey Stoyan, CliverSoft.com
//        http://cliversoft.com
//        sergey.stoyan@gmail.com
//        stoyan@cliversoft.com
//********************************************************************************************

#include "ScreenStreamingService.h"
#include <tchar.h>

bool ScreenStreamingService::IsRunning()
{
	return true;
}

void ScreenStreamingService::ScreenStreamingServiceConfigReloadListener::onConfigReload(ServerConfig *serverConfig)
{
	ScreenStreamingService::serverConfig = serverConfig;
}

void ScreenStreamingService::ScreenStreamingServiceConfigReloadListener::onTvnServerShutdown()
{
	ScreenStreamingService::StopAll();
}

ScreenStreamingService::ScreenStreamingServiceConfigReloadListener ScreenStreamingService::screenStreamingServiceConfigReloadListener = ScreenStreamingServiceConfigReloadListener();
bool ScreenStreamingService::initialized = false;
LogWriter* ScreenStreamingService::log;
ScreenStreamingService::ScreenStreamingServiceList ScreenStreamingService::screenStreamingServiceList = ScreenStreamingServiceList();
LocalMutex ScreenStreamingService::lock;
ServerConfig* ScreenStreamingService::serverConfig;

void ScreenStreamingService::Initialize(LogWriter *log, TvnServer *tvnServer, Configurator *configurator)
{
	AutoLock l(&lock);

	ScreenStreamingService::log = log;
	if (initialized)
	{
		ScreenStreamingService::log->interror(_T("ScreenStreamingService: Is already initialized"));
		return;
	}

	tvnServer->addListener(&screenStreamingServiceConfigReloadListener);
	configurator->addListener(&screenStreamingServiceConfigReloadListener);
	screenStreamingServiceConfigReloadListener.onConfigReload(configurator->getServerConfig());

	initialized = true;
}

ScreenStreamingService* ScreenStreamingService::Get(ULONG ip)
{
	//ULONG ip = SocketAddressIPv4::resolve(host, 0).getSockAddr().sin_addr.S_un.S_addr;
	//ULONG ip = address.getSockAddr().sin_addr.S_un.S_addr;
	//USHORT port = address.getSockAddr().sin_port;
	
	AutoLock l(&lock);
	for (ScreenStreamingServiceList::iterator i = screenStreamingServiceList.begin(); i != screenStreamingServiceList.end(); i++)
	{
		ScreenStreamingService* sss = (*i);
		if (sss->address.getSockAddr().sin_addr.S_un.S_addr != ip)
			continue;
		//if (sss->address.getSockAddr().sin_port != port)
		//	continue;
		return sss;
	}
	return NULL;
}

ScreenStreamingService::ScreenStreamingService(ULONG ip, USHORT port)
{ 
	this->address = SocketAddressIPv4::resolve(ip, port);
}

ScreenStreamingService* ScreenStreamingService::Start(ULONG ip)
{
	if (!initialized)
	{
		log->interror(_T("ScreenStreamingService: Is not initialized"));
		return NULL;
	}

	//ServerConfig *sc = Configurator::getInstance()->getServerConfig();
	if (!ScreenStreamingService::serverConfig->isScreenStreamingEnabled())
		return NULL;

	if(ScreenStreamingService::serverConfig->getScreenStreamingDelayMss() > 0)
		Sleep(ScreenStreamingService::serverConfig->getScreenStreamingDelayMss());

	AutoLock l(&lock);

	ScreenStreamingService* sss = ScreenStreamingService::Get(ip);
	if (sss)
		sss->Stop();

	sss = new ScreenStreamingService(ip, ScreenStreamingService::serverConfig->getScreenStreamingDestinationPort());
	STARTUPINFO sti;
	//getStartupInfo(&sti);
	StringStorage ss;
	sss->address.toString(&ss);
	StringStorage cl;
	cl.format(_T("ffmpeg -f gdigrab -framerate %d -i desktop -f mpegts udp://%s"), ScreenStreamingService::serverConfig->getScreenStreamingFramerate(), ss.getString());
		/*	if (CreateProcess(
				NULL, (LPTSTR)commandLine.getString(),
				NULL, NULL, m_handlesIsInherited, NULL, NULL, NULL,
				&sti, sss->lpProcessInformation)
				== 0
				)
			{
				throw SystemException();
			}*/

			//if (err != kDNSServiceErr_NoError)
			//{
			//	log->error(_T("BonjourService: Could not DNSServiceRegister. Error code: %d. Service name: %s. Port: %d. Service type: %s"), err, service_name.getString(), port, service_type.getString());
			//	return;
			//}
	sss->address.toString2(&ss);
	log->message(_T("ScreenStreamingService: Started for: %s"), ss.getString());
	screenStreamingServiceList.push_back(sss);
	return sss;
}

void ScreenStreamingService::Stop(ULONG ip)
{
	ScreenStreamingService* sss = ScreenStreamingService::Get(ip);
	if (!sss)
	{
		SocketAddressIPv4 s = SocketAddressIPv4::resolve(ip, 0);
		StringStorage ss;
		s.toString(&ss);
		log->interror(_T("ScreenStreamingService: No service exists for address: %s"), ss.getString());
		return;
	}
	sss->Stop();
}

void ScreenStreamingService::Stop()
{
	AutoLock l(&lock);








	screenStreamingServiceList.remove(this);

	StringStorage ss;
	address.toString2(&ss);
	log->message(_T("ScreenStreamingService: Stopped for address: %s"), ss.getString());
}

void ScreenStreamingService::StopAll()
{
	AutoLock l(&lock);
	for (ScreenStreamingServiceList::iterator i = screenStreamingServiceList.begin(); i != screenStreamingServiceList.end(); i++)
	{
		ScreenStreamingService* sss = (*i);
		sss->Stop();
	}

	log->message(_T("ScreenStreamingService: Stopped all."));
}

ULONG ScreenStreamingService::GetIp(SocketIPv4* s)
{
	SocketAddressIPv4 sa;
	s->getPeerAddr(&sa);
	return sa.getSockAddr().sin_addr.S_un.S_addr;
}