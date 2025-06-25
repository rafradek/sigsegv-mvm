#ifndef _INCLUDE_SIGSEGV_UTIL_NETMESSAGES_H_
#define _INCLUDE_SIGSEGV_UTIL_NETMESSAGES_H_

class CNetMessage : public INetMessage
{
public:
	virtual ~CNetMessage() {}
    virtual void	SetNetChannel(INetChannel * netchan) {}; // netchannel this message is from/for
    virtual void	SetReliable( bool state ) {};	// set to true if it's a reliable message
    
    virtual bool	Process( void ) { return true; }; // calles the recently set handler to process this message
    
    virtual	bool	ReadFromBuffer( bf_read &buffer ) { return true; } // returns true if parsing was OK
    virtual bool Write(bf_write &buffer) = 0;
    virtual	bool WriteToBuffer(bf_write &buffer) {
        buffer.WriteUBitLong(this->GetType(), 6);
        return Write(buffer); 
    }
        
    virtual bool	IsReliable( void ) const { return true; }  // true, if message needs reliable handling
    
    virtual int				GetGroup( void ) const {return 0;};	// returns net message group of this message
    virtual INetChannel		*GetNetChannel( void ) const { return nullptr;}
    virtual const char		*ToString( void ) const { return ""; } // returns a human readable string about message content
    virtual bool	BIncomingMessageForProcessing( double dblNetTime, int numBytes ) { return true; }

    virtual size_t GetSize() const { return 0; }
};

class SetConVarMessage : public CNetMessage
{
public:
    virtual	bool Write(bf_write &buffer) {
        buffer.WriteByte(values.size());
        for (auto &pair : values) {
            buffer.WriteString(pair.first.c_str());
            buffer.WriteString(pair.second.c_str());
        }
        return true; 
    }
    virtual int				GetType( void ) const { return 5; }; // returns module specific header tag eg svc_serverinfo
    virtual const char		*GetName( void ) const { return "net_SetConVar"; }	// returns network message name, eg "svc_serverinfo"
    std::unordered_map<std::string, std::string> values;
};


class CmdMessage : public CNetMessage
{
public:
    virtual	bool Write(bf_write &buffer) {
        return buffer.WriteString(dest.c_str());
    }
    
    virtual int				GetType( void ) const { return 4; }; // returns module specific header tag eg svc_serverinfo
    virtual const char		*GetName( void ) const { return "net_Message"; }	// returns network message name, eg "svc_serverinfo"

    std::string dest;
};

#endif