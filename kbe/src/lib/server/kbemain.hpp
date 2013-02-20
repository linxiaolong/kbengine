/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __KBEMAIN__
#define __KBEMAIN__
#include "helper/memory_helper.hpp"

#include "serverapp.hpp"
#include "cstdkbe/cstdkbe.hpp"
#include "helper/debug_helper.hpp"
#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "server/componentbridge.hpp"
#include "server/serverinfos.hpp"
#include "resmgr/resmgr.hpp"

#if KBE_PLATFORM == PLATFORM_WIN32
#include "helper/crashhandler.hpp"
#endif

namespace KBEngine{

inline void START_MSG(const char * name, uint64 appuid)
{
	ServerInfos serverInfo;
	
	INFO_MSG(boost::format("---- %1% "
			"Version: %2%. "
			"Config: %3%. "
			"Built: %4% %5%. "
			"AppUID: %6%. "
			"UID: %7%. "
			"PID: %8% ----\n") %
		name % KBEVersion::versionString().c_str() %
		KBE_CONFIG % __TIME__ % __DATE__ %
		appuid % getUserUID() % getProcessPID() );
	
	INFO_MSG(boost::format("Server %1%: %2% with %3% RAM\n") %
		serverInfo.serverName().c_str() %
		serverInfo.cpuInfo().c_str() %
		serverInfo.memInfo().c_str() );
}

inline void loadConfig()
{
	Resmgr::getSingleton().initialize();

	// "../../res/server/kbengine_defs.xml"
	g_kbeSrvConfig.loadConfig("server/kbengine_defs.xml");

	// "../../../demo/res/server/kbengine.xml"
	g_kbeSrvConfig.loadConfig("server/kbengine.xml");
}

template <class SERVER_APP>
int kbeMainT(int argc, char * argv[], COMPONENT_TYPE componentType, 
			 int32 extlisteningPort_min = -1, int32 extlisteningPort_max = -1, const char * extlisteningInterface = "",
			 int32 intlisteningPort = 0, const char * intlisteningInterface = "")
{
	g_componentType = componentType;
	DebugHelper::initHelper(componentType);
	INFO_MSG( "-----------------------------------------------------------------------------------------\n\n\n");
	
	Resmgr::getSingleton().pirnt();

	Mercury::EventDispatcher dispatcher;
	DebugHelper::getSingleton().pDispatcher(&dispatcher);

	const ChannelCommon& channelCommon = g_kbeSrvConfig.channelCommon();

	Mercury::g_SOMAXCONN = g_kbeSrvConfig.tcp_SOMAXCONN(g_componentType);

	Mercury::NetworkInterface networkInterface(&dispatcher, 
		extlisteningPort_min, extlisteningPort_max, extlisteningInterface, 
		channelCommon.extReadBufferSize, channelCommon.extWriteBufferSize,
		(intlisteningPort != -1) ? htons(intlisteningPort) : -1, intlisteningInterface,
		channelCommon.intReadBufferSize, channelCommon.intWriteBufferSize);
	
	DebugHelper::getSingleton().pNetworkInterface(&networkInterface);

	g_kbeSrvConfig.updateInfos(true, componentType, g_componentID, 
			networkInterface.intaddr(), networkInterface.extaddr());
	
	if(getUserUID() <= 0)
	{
		WARNING_MSG(boost::format("invalid UID(%1%) <= 0, please check UID for environment!\n") % getUserUID());
	}

	Componentbridge* pComponentbridge = new Componentbridge(networkInterface, componentType, g_componentID);
	SERVER_APP app(dispatcher, networkInterface, componentType, g_componentID);
	START_MSG(COMPONENT_NAME_EX(componentType), g_componentID);

	if(!app.initialize())
	{
		ERROR_MSG("app::initialize is error!\n");
		SAFE_RELEASE(pComponentbridge);
		app.finalise();
		return -1;
	}
	
	INFO_MSG(boost::format("---- %1% is running ----\n") % COMPONENT_NAME_EX(componentType));

	int ret = app.run();

	SAFE_RELEASE(pComponentbridge);
	app.finalise();
	INFO_MSG(boost::format("%1% has shut down.\n") % COMPONENT_NAME_EX(componentType));
	return ret;
}

#if KBE_PLATFORM == PLATFORM_WIN32
#define KBENGINE_MAIN																									\
kbeMain(int argc, char* argv[]);																						\
int main(int argc, char* argv[])																						\
{																														\
	loadConfig();																										\
	g_componentID = genUUID64();																						\
	char dumpname[MAX_BUF] = {0};																						\
	kbe_snprintf(dumpname, MAX_BUF, "%"PRAppID, g_componentID);															\
	KBEngine::exception::installCrashHandler(1, dumpname);																\
	int retcode = -1;																									\
	THREAD_TRY_EXECUTION;																								\
	retcode = kbeMain(argc, argv);																						\
	THREAD_HANDLE_CRASH;																								\
	return retcode;																										\
}																														\
int kbeMain
#else
#define KBENGINE_MAIN																									\
kbeMain(int argc, char* argv[]);																						\
int main(int argc, char* argv[])																						\
{																														\
	loadConfig();																										\
	g_componentID = genUUID64();																						\
	return kbeMain(argc, argv);																							\
}																														\
int kbeMain
#endif
}

#endif // __KBEMAIN__
