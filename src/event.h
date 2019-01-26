/*******************************************************************************
Copyright (c) 2014, Jan Koester jan.koester@gmx.net
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

#include "exception.h"
#include "connections.h"
#include "os/os.h"
#include <config.h>

#ifdef Windows
  #include <Windows.h>
  #include <mswsock.h>
  #include <Strsafe.h>
#endif

#ifdef EVENT_KQUEUE
  #include <sys/event.h>
#endif

#ifndef EVENT_H
#define EVENT_H

namespace libhttppp {
    class ThreadPool;
#ifdef MSVC
	class __declspec(dllexport)  Event {
#else
	class __attribute__ ((visibility ("default"))) Event {
#endif
	public:
		Event(ServerSocket *serversocket);
		virtual ~Event();

        class WorkerContext {
        private:
            WorkerContext();
            ~WorkerContext();
             /*Linking to Events*/
            Event                 *_CurEvent;
            Thread               *_CurThread;
            WorkerContext  *_nextWorkerContext;
            friend class Event;
        };
        
        WorkerContext *addWorkerContext();
        WorkerContext *delWorkerContext(WorkerContext *delwrkctx);
        
        class ConnectionContext {
        public:
            ConnectionContext     *nextConnectionContext();
        private:
            ConnectionContext();
            ~ConnectionContext();
			
#ifdef EVENT_IOCP
			/*Acceptex for iocp*/
			LPFN_ACCEPTEX          fnAcceptEx;
			/*WSA Ovlerlapped*/
			class IOCPConnectionData : protected ConnectionData {
			protected:
				IOCPConnectionData(const char*data, size_t datasize, WSAOVERLAPPED *overlapped);
				~IOCPConnectionData();
			private:
				WSAOVERLAPPED         *_Overlapped;
				IOCPConnectionData    *_nextConnectionData;
				friend class           Event;
				friend class           ConnectionContext;
			};

			class IOCPConnection : public Connection {
			public:
				IOCPConnection();
				~IOCPConnection();
				IOCPConnectionData *addRecvQueue(const char data[BLOCKSIZE],size_t datasize, WSAOVERLAPPED *overlapped);
			private:
				/*Incomming Data*/
				IOCPConnectionData *_ReadDataFirst;
				IOCPConnectionData *_ReadDataLast;
			};

#elif EVENT_KQUEUE
	    /*counter for Events*/
            ssize_t                  _EventCounter;
#endif
            /*Indefier Connection*/
#ifndef EVENT_IOCP
			Connection             *_CurConnection;
#else
			IOCPConnection         *_CurConnection;
#endif
            /*current Mutex*/
            Lock                     *_Lock;
            /*next entry*/
            ConnectionContext      *_nextConnectionContext;
            friend class Event;
        };
   
        void addConnectionContext(ConnectionContext **addcon);
        void delConnectionContext(ConnectionContext *delctx,ConnectionContext **nextcxt);
        
    /*API Events*/
    virtual void RequestEvent(Connection *curcon);
    virtual void ResponseEvent(libhttppp::Connection *curcon);
    virtual void ConnectEvent(libhttppp::Connection *curcon);
    virtual void DisconnectEvent(Connection *curcon);
    /*Run Mainloop*/
    virtual void runEventloop();

#ifdef EVENT_IOCP
	static DWORD WINAPI WorkerThread(LPVOID WorkThreadContext);
#else
    static void *WorkerThread(void *wrkevent);
#endif

#ifdef Windows
    static BOOL WINAPI CtrlHandler(DWORD dwEvent);
#else
    static  void  CtrlHandler(int signum);
#endif

  private:
#ifdef EVENT_EPOLL
	int                            _epollFD;
#elif EVENT_KQUEUE
	int                            _Kq;
	struct kevent        *_Events;
#elif EVENT_IOCP
	HANDLE              _IOCP;
	WSAEVENT            _hCleanupEvent[1];
	CRITICAL_SECTION    _CriticalSection;
#endif
    
    /*Connection Context helper*/
    ConnectionContext *_firstConnectionContext;
    ConnectionContext *_lastConnectionContext;
    
    /*Threadpools*/
    ThreadPool              *_WorkerPool;
    WorkerContext           *_firstWorkerContext;
    WorkerContext           *_lastWorkerContext;   
    Lock                    *_Lock;
    
    ServerSocket            *_ServerSocket;
    static bool              _EventEndloop;
    static bool              _EventRestartloop;
  };
}

#endif
