//********************************************************************************************
//Author: Sergey Stoyan, CliverSoft.com
//        http://cliversoft.com
//        sergey.stoyan@gmail.com
//        stoyan@cliversoft.com
//********************************************************************************************

#ifndef _SCREEN_STREAMING_SERVICE_H_
#define _SCREEN_STREAMING_SERVICE_H_

#include "server-config-lib/Configurator.h"
#include "TvnServer.h"

/*
WISHES:

*/

/*
This service creates a video stream to the client INDEPENDENTLY on RFB connection.
*/

using namespace std;

class ScreenStreamingService
{
	typedef list<ScreenStreamingService *> ScreenStreamingServiceList;

	class ScreenStreamingServiceConfigReloadListener : public ConfigReloadListener, public TvnServerListener
	{
	public:
		void onConfigReload(ServerConfig* serverConfig);
		void onTvnServerShutdown();
	};

public:
	static void Initialize(LogWriter* log, TvnServer* tvnServer, Configurator* configurator);

	static ScreenStreamingService* Start(ULONG ip);
	static ScreenStreamingService* Get(ULONG ip);
	static void Stop(ULONG ip);
	void Stop();
	bool IsRunning();
	static void StopAll();

	static ULONG GetIp(SocketIPv4* s);

private:
	ScreenStreamingService(ULONG ip, USHORT port);

	LPPROCESS_INFORMATION lpProcessInformation;
	SocketAddressIPv4 address; 

	static LocalMutex lock;
	static ScreenStreamingServiceList screenStreamingServiceList;
	static ScreenStreamingServiceConfigReloadListener screenStreamingServiceConfigReloadListener;
	static bool initialized;
	static LogWriter* log;
	static ServerConfig* serverConfig;
};


#endif