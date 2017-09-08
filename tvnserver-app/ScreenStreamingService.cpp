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
	initialized = true;
	screenStreamingServiceConfigReloadListener.onConfigReload(configurator->getServerConfig());
}

ScreenStreamingService* ScreenStreamingService::Get(TCHAR* host)
{
	ULONG ip = SocketAddressIPv4::resolve(host, 0).getSockAddr().sin_addr.S_un.S_addr;
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

ScreenStreamingService::ScreenStreamingService(TCHAR* host, USHORT port)
{ 
	this->address = SocketAddressIPv4(host, port);
}

ScreenStreamingService* ScreenStreamingService::Start(TCHAR* host)
{
	if (!initialized)
	{
		log->interror(_T("ScreenStreamingService: Is not initialized"));
		return NULL;
	}

	AutoLock l(&lock);

	//ServerConfig *sc = Configurator::getInstance()->getServerConfig();
	if (!ScreenStreamingService::serverConfig->isScreenStreamingEnabled())
		return NULL;

	ScreenStreamingService* sss = ScreenStreamingService::Get(host);
	if (sss)
		sss->Stop();

	sss = new ScreenStreamingService(host, ScreenStreamingService::serverConfig->getScreenStreamingDestinationPort());
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
		StringStorage ss;
	sss->address.toString(&ss);
	log->message(_T("ScreenStreamingService: Started for: %s"), ss.getString());
	screenStreamingServiceList.push_back(sss);
	return sss;
}

void ScreenStreamingService::Stop(TCHAR* host)
{
	ScreenStreamingService* sss = ScreenStreamingService::Get(host);
	if (!sss)
	{
		StringStorage ss;
		sss->address.toString(&ss);
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
	address.toString(&ss);
	log->message(_T("ScreenStreamingService: Stopped for address: %s", ss.getString()));
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

SocketAddressIPv4 getAddress(SocketIPv4* s)
{
	SocketAddressIPv4 sa;
	s->getPeerAddr(&sa);
	sa.getSockAddr().sin_addr.S_un.S_addr;
}