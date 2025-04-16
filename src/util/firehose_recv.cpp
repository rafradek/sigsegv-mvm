#include "util/firehose_recv.h"


const addrinfo *FirehoseRecv::GetAddrHints() const
{
	static const addrinfo hints = {
		AI_PASSIVE,  // .ai_flags
		AF_INET6,    // .ai_family
		SOCK_STREAM, // .ai_socktype
		IPPROTO_TCP, // .ai_protocol
	};
	return &hints;
}


size_t FirehoseRecv::Recv(size_t len, uint8_t *dst, bool peek)
{
	if (this->m_Socket == INVALID_SOCKET || len == 0 || dst == nullptr) {
		return 0;
	}
	
	size_t actual;
	
	{
		std::lock_guard<std::mutex> guard(this->m_DataMutex);
		
		actual = std::min(len, this->m_Data.size());
		
#if defined _MSC_VER // avoid C4996
		std::copy_n(this->m_Data.begin(), actual, stdext::make_unchecked_array_iterator(dst));
#else
		std::copy_n(this->m_Data.begin(), actual, dst);
#endif
		if (!peek) {
			this->m_Data.erase(this->m_Data.begin(), this->m_Data.begin() + actual);
		}
	}
	
	return actual;
}

size_t FirehoseRecv::Available()
{
	std::lock_guard<std::mutex> guard(this->m_DataMutex);
	
	return this->m_Data.size();
}

void FirehoseRecv::Skip(size_t len)
{
	std::lock_guard<std::mutex> guard(this->m_DataMutex);
	
	size_t actual = std::min(len, this->m_Data.size());
	
	this->m_Data.erase(this->m_Data.begin(), this->m_Data.begin() + actual);
}


bool FirehoseRecv::Bind()
{
	for (addrinfo *addr = this->GetAddrs(); addr != nullptr; addr = addr->ai_next) {
		if (bind(this->m_Socket, addr->ai_addr, addr->ai_addrlen) == 0) {
			DevMsg("FirehoseRecv::Bind: bound\n");
			return true;
		}
		
		Warning("FirehoseRecv::Bind: bind() failed: %s\n", this->GetErrorMessage());
	}
	
	return false;
}

bool FirehoseRecv::Listen()
{
	if (listen(this->m_Socket, 1) != 0) {
		Warning("FirehoseRecv::Listen: listen() failed: %s\n", this->GetErrorMessage());
		return false;
	} else {
		DevMsg("FirehoseRecv::Listen: listening\n");
		return true;
	}
}


void FirehoseRecv::CreateThread()
{
	this->m_DoneFlag.test_and_set();
	
	this->m_Thread = new std::thread(&FirehoseRecv::ThreadLoop, this);
}

void FirehoseRecv::JoinThread()
{
	if (this->m_Thread != nullptr) {
		DevMsg("[Main] setting done flag\n");
		this->m_DoneFlag.clear();
		
		DevMsg("[Main] joining thread\n");
		this->m_Thread->join();
		DevMsg("[Main] thread joined\n");
		
		delete this->m_Thread;
		this->m_Thread = nullptr;
		
		
		// use a signal to kick the thread out of accept() or read() or whatever
		
		// TODO
	}
}


void FirehoseRecv::ThreadLoop()
{
	if (this->m_Socket == INVALID_SOCKET) {
		return;
	}
	
	DevMsg("[Thread] begin\n");
	while (true) {
		if (this->m_Client == INVALID_SOCKET) {
			if (!this->m_DoneFlag.test_and_set()) {
				break;
			}
			
			DevMsg("[Thread] will accept()\n");
			this->InternalAccept();
		} else {
//			DevMsg("[Thread] will recv()\n");
			this->InternalRecv();
		}
	}
	DevMsg("[Thread] end\n");
}


void FirehoseRecv::InternalAccept()
{
	int result = accept(this->m_Socket, nullptr, nullptr);
	if (result != INVALID_SOCKET) {
		DevMsg("FirehoseRecv::InternalAccept: connection opened (fd %d)\n", result);
		this->m_Client = result;
	} else {
		// TODO: WinSock error checks
		// - WSAEINTR
#if defined _WINDOWS
		Warning("FirehoseRecv::InternalAccept: accept() failed: %s\n", this->GetErrorMessage());
#else
		if (errno == EINVAL) {
			DevMsg("FirehoseRecv::InternalAccept: socket closed\n");
		} else if (errno != EAGAIN) {
			Warning("FirehoseRecv::InternalAccept: accept() failed: %s\n", this->GetErrorMessage());
		}
#endif
	}
}


void FirehoseRecv::InternalRecv()
{
	this->m_RecvBuf.resize(1024 * 1024);
	
#if defined _WINDOWS
	int result = recv(this->m_Client, reinterpret_cast<char *>(this->m_RecvBuf.data()), this->m_RecvBuf.size(), 0);
#else
	ssize_t result = recv(this->m_Client, this->m_RecvBuf.data(), this->m_RecvBuf.size(), 0);
#endif
	if (result == SOCKET_ERROR) {
		// TODO: WinSock error checks
#if defined _WINDOWS
		Warning("FirehoseRecv::InternalRecv: recv() failed: %s\n", this->GetErrorMessage());
#else
		if (errno != EAGAIN) {
			Warning("FirehoseRecv::InternalRecv: recv() failed: %s\n", this->GetErrorMessage());
		}
#endif
		return;
	} else if (result == 0) {
		// BUG: why are we getting return value of 0 in cases where we merely timeout
		// and the connection hasn't been closed?
		DevMsg("FirehoseRecv::InternalRecv: connection closed\n");
#if defined _WINDOWS
		closesocket(this->m_Client);
#else
		close(this->m_Client);
#endif
		this->m_Client = INVALID_SOCKET;
		return;
	}
	
	size_t bytes = result;
	DevMsg("FirehoseRecv::InternalRecv: received %u bytes\n", bytes);
	
	// BUG: the copy_n operation isn't actually increasing the size of m_Data
	{
		std::lock_guard<std::mutex> guard(this->m_DataMutex);
		this->m_Data.insert(this->m_Data.end(), this->m_RecvBuf.begin(), this->m_RecvBuf.begin() + bytes);
	}
}


FirehoseRecv *recver = nullptr;

CON_COMMAND(firehose_recv_test_begin, "")
{
	if (recver != nullptr) {
		return;
	}
	
	recver = new FirehoseRecv(30001);
}

CON_COMMAND(firehose_recv_test_end, "")
{
	delete recver;
	recver = nullptr;
}
