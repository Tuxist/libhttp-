/*******************************************************************************
Copyright (c) 2019, Jan Koester jan.koester@gmx.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <Windows.h>
#include <mswsock.h>
#include <Strsafe.h>

#include "config.h"

#include "../../../connections.h"
#include "../../os.h"

#ifndef IOCP_H
#define IOCP_H

namespace libhttppp {
	class IOCP {
	public:
		IOCP(ServerSocket *serversocket);
		~IOCP();
		void runEventloop();
		static void *WorkerThread(void *wrkevent);
		/*	protected:
		static BOOL WINAPI CtrlHandler(DWORD dwEvent);
		static bool _EventEndloop;
		static bool _EventRestartloop;
	private:

		void addConnectionContext(ConnectionContext **addcon);
		void delConnectionContext(ConnectionContext *delctx, ConnectionContext **nextcxt);

		WorkerContext *addWorkerContext();
		WorkerContext *delWorkerContext(WorkerContext *delwrkctx);

		HANDLE              _IOCP;
		WSAEVENT            _hCleanupEvent[1];
		CRITICAL_SECTION    _CriticalSection;

		ConnectionContext *_firstConnectionContext;
		ConnectionContext *_lastConnectionContext;

		ThreadPool              *_WorkerPool;
		WorkerContext           *_firstWorkerContext;
		WorkerContext           *_lastWorkerContext;
		Lock                    *_Lock;

		ServerSocket            *_ServerSocket;#*/
	};
};

#endif
