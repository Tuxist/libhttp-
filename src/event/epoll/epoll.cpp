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


#include <systempp/syscall.h>
#include <systempp/sysbits.h>
#include <systempp/sysconsole.h>

#include <config.h>

#include "../../exception.h"
#include "../../connections.h"

#define READEVENT 0
#define SENDEVENT 1

#define DEBUG_MUTEX

#include "epoll.h"
#include "threadpool.h"

libhttppp::EPOLL::EPOLL(libsystempp::ServerSocket* serversocket) {
    HTTPException httpexception;
    _ServerSocket=serversocket;
}

libhttppp::EPOLL::~EPOLL(){
}

void libhttppp::EPOLL::LockEventPool(){
    _ConLock.lock();
}

void libhttppp::EPOLL::UnlockEventPool(){
    _ConLock.unlock();
}
        
int libhttppp::EPOLL::LockConnection(int des) {
    _ConLock.lock();
    Connection *curct=(Connection*)_Events[des].data;
    if(curct){
        if(curct->ConnectionLock.try_lock()){
             _ConLock.unlock();
            return LockState::LOCKED;
        }
    }
    _ConLock.unlock();
    return LockState::NOLOCK;
}

void libhttppp::EPOLL::UnlockConnection(int des){
    HTTPException excep;
    _ConLock.lock();
    Connection *curct=(Connection*)_Events[des].data;
    if(curct){
        curct->ConnectionLock.unlock();
    }
    _ConLock.unlock();
}

const char * libhttppp::EPOLL::getEventType(){
    return "EPOLL";
}

void libhttppp::EPOLL::initEventHandler(){
    HTTPException httpexception;
    try{
        _ServerSocket->setnonblocking();
        _ServerSocket->listenSocket();
    }catch(HTTPException &e){
        throw e;
    }
    
    struct epoll_event setevent= (struct epoll_event) {
        0
    };
 
    _epollFD =syscall1(__NR_epoll_create1,0);

    if (_epollFD < 0) {
        httpexception[HTTPException::Critical]<<"initEventHandler:" << "can't create epoll";
        throw httpexception;
    }

    setevent.events = EPOLLIN;

    if (syscall4(__NR_epoll_ctl,_epollFD,EPOLL_CTL_ADD,
        _ServerSocket->getSocket(),(unsigned long)&setevent) < 0) {
        httpexception[HTTPException::Critical] << "initEventHandler: can't create epoll";
        throw httpexception;
    }
    
    _Events = new epoll_event[_ServerSocket->getMaxconnections()];
    for(int i=0; i<_ServerSocket->getMaxconnections(); ++i)
        _Events[i].data = (__u64)nullptr;
}

int libhttppp::EPOLL::waitEventHandler(){
    int n = syscall4(__NR_epoll_wait,_epollFD,
                     (unsigned long)_Events,_ServerSocket->getMaxconnections(), -1);
    if(n<0) {
        HTTPException httpexception;
        httpexception[HTTPException::Critical] << "initEventHandler: epoll wait failure";
        throw httpexception;
    }
    return n;
}

int libhttppp::EPOLL::StatusEventHandler(int des){
    HTTPException httpexception;
    Connection *curct=(Connection*)_Events[des].data;
    if(!curct)
        return EventHandlerStatus::EVNOTREADY;
    if(curct->getSendSize()>0)
            return EventHandlerStatus::EVOUT;
    return EventHandlerStatus::EVIN;
}

void libhttppp::EPOLL::ConnectEventHandler(int des) {
    HTTPException httpexception;
    Connection* curct = new Connection(_ServerSocket,this);
    curct->ConnectionLock.lock();
    try {
        /*will create warning debug mode that normally because the check already connection
         * with this socket if getconnection throw they will be create a new one
         */
        _ServerSocket->acceptEvent(curct->getClientSocket());
        curct->getClientSocket()->setnonblocking();
        struct epoll_event *setevent=new epoll_event{ 0 };
        setevent->events = EPOLLIN;
        setevent->data =(__u64) curct;
        curct->ConnectionPtr = setevent; 
        if (syscall4(__NR_epoll_ctl,_epollFD,EPOLL_CTL_ADD,
            curct->getClientSocket()->getSocket(),(unsigned long) setevent) < 0) {
            delete setevent;
            httpexception[HTTPException::Error] << "ConnectEventHandler: can't add socket to epoll";
            throw httpexception;
        }
        ConnectEvent(curct);
    }catch (HTTPException& e) {
        delete curct;
        _Events[des].data=(__u64)nullptr;
        throw e;
    }
    curct->ConnectionLock.unlock();
}

void libhttppp::EPOLL::ReadEventHandler(int des){
    HTTPException httpexception;
    Connection *curct=((Connection*)_Events[des].data);
    char buf[BLOCKSIZE];
    if(!curct){
        httpexception[HTTPException::Error] << "ReadEventHandler: no valid data !";
        throw httpexception;
    }
    int rcvsize=_ServerSocket->recvData(curct->getClientSocket(),&buf,BLOCKSIZE);
    curct->addRecvQueue(buf,rcvsize);
    RequestEvent(curct);
}

void libhttppp::EPOLL::WriteEventHandler(int des){
    HTTPException httpexception;
    Connection *curct=((Connection*)_Events[des].data);
    if(!curct || !curct->getSendData()){
         httpexception[HTTPException::Error] << "WriteEventHandler: no valid data !";
         throw httpexception;
    }
    ssize_t sended=_ServerSocket->sendData(curct->getClientSocket(),
                                       (void*)curct->getSendData()->getData(),
                                       curct->getSendData()->getDataSize());
    curct->resizeSendQueue(sended);
    if(!curct->getSendData())
        _setEpollEvents(curct,EPOLLIN);
    ResponseEvent(curct);
}

void libhttppp::EPOLL::CloseEventHandler(int des){
    HTTPException httpexception;
    try {
        Connection *curct=((Connection*)_Events[des].data);
        if(!curct){
            httpexception[HTTPException::Error] << "CloseEvent empty DataPtr!";
            throw httpexception;
        }
        struct epoll_event *setevent=(struct epoll_event*)curct->ConnectionPtr;
        int ect=syscall4(__NR_epoll_ctl,_epollFD,EPOLL_CTL_DEL,curct->getClientSocket()->getSocket(), 
                         (unsigned long)setevent);
        if(ect<0) {
            httpexception[HTTPException::Error] << "CloseEvent can't delete Connection from epoll";
            throw httpexception;
        }
        DisconnectEvent(curct);
        delete setevent;
        delete curct;
        _Events[des].data=(__u64)nullptr;
        httpexception[HTTPException::Note] << "CloseEventHandler: Connection shutdown!";
    } catch(HTTPException &e) {
        httpexception[HTTPException::Warning] << "CloseEventHandler: Can't do Connection shutdown!";
    }
}


/*API Events*/

void libhttppp::EPOLL::RequestEvent(Connection *curcon){
    return;
};

void libhttppp::EPOLL::ResponseEvent(Connection *curcon){
    return;
};

void libhttppp::EPOLL::ConnectEvent(Connection *curcon){
    return;
};

void libhttppp::EPOLL::DisconnectEvent(Connection *curcon){
    return;
};

void libhttppp::EPOLL::sendReady(libhttppp::Connection* curcon, bool ready){
    if(ready){
        _setEpollEvents(curcon,EPOLLIN | EPOLLOUT);
    }else{
        _setEpollEvents(curcon,EPOLLIN);
    }
}


void libhttppp::EPOLL::_setEpollEvents(Connection *curcon,int events){
    HTTPException httpexception;
    struct epoll_event *setevent=(struct epoll_event *)curcon->ConnectionPtr;
    setevent->events = events;
    if (syscall4(__NR_epoll_ctl,_epollFD,EPOLL_CTL_MOD, 
        curcon->getClientSocket()->getSocket(), (unsigned long)setevent) < 0) {
        httpexception[HTTPException::Error] << "_setEpollEvents: can change socket!";
        _ConLock.unlock();
        throw httpexception;
    }
}
