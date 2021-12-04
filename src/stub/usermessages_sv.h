#ifndef _INCLUDE_SIGSEGV_STUB_USERMESSAGES_SV_H_
#define _INCLUDE_SIGSEGV_STUB_USERMESSAGES_SV_H_


#include "link/link.h"
#include "stub/baseplayer.h"
//#include "util/iterate.h" // <-- very bad to include util/iterate.h in a header!


class CRecipientFilter : public IRecipientFilter
{
public:
	CRecipientFilter(bool reliable = false, bool init_msg = false) :
		m_bReliable(reliable), m_bInitMessage(init_msg) {}
	virtual ~CRecipientFilter() {};
	
	virtual bool IsReliable() const override    { return this->m_bReliable;    }
	virtual bool IsInitMessage() const override { return this->m_bInitMessage; }
	
	virtual int GetRecipientCount() const override { return this->m_Recipients.size(); }
	virtual int GetRecipientIndex(int slot) const override
	{
		if (slot >= 0 && slot < (int)this->m_Recipients.size()) {
			return this->m_Recipients[slot];
		} else {
			return -1;
		}
	}
	
	void Reset()
	{
		this->m_bReliable    = false;
		this->m_bInitMessage = false;
		this->m_Recipients.clear();
	}

	void CopyFrom   (const IRecipientFilter &filter);
	
	void MakeReliable()    { this->m_bReliable    = true; }
	void MakeInitMessage() { this->m_bInitMessage = true; }
	
	void AddRecipient   (const CBasePlayer *player);
	void RemoveRecipient(const CBasePlayer *player);
	
	void AddAllPlayers();
	void RemoveAllRecipients();
	
	void AddRecipientsByPVS   (const Vector& origin);
	void RemoveRecipientsByPVS(const Vector& origin);
	
	void AddRecipientsByPAS   (const Vector& origin);
	void RemoveRecipientsByPAS(const Vector& origin);
	
//	void AddRecipientsByTeam   (CTeam *team); // TODO
//	void RemoveRecipientsByTeam(CTeam *team); // TODO
	
	void AddPlayersFromBitMask   (const CBitVec<ABSOLUTE_PLAYER_LIMIT>& bits);
	void RemovePlayersFromBitMask(const CBitVec<ABSOLUTE_PLAYER_LIMIT>& bits);
	
private:
	bool m_bReliable;
	bool m_bInitMessage;
	std::vector<int> m_Recipients;
};

inline void CRecipientFilter::CopyFrom(const IRecipientFilter &filter)
{
	int count = filter.GetRecipientCount();
	for (int i = 0; i < count; i++) {
		int id = filter.GetRecipientIndex(i);
		if (std::find(this->m_Recipients.begin(), this->m_Recipients.end(), id) != this->m_Recipients.end()) continue;

		this->m_Recipients.push_back(id);
	}
}

inline void CRecipientFilter::AddRecipient(const CBasePlayer *player)
{
	if (player == nullptr) return;
	
	int idx = ENTINDEX(player);
	if (std::find(this->m_Recipients.begin(), this->m_Recipients.end(), idx) != this->m_Recipients.end()) return;
	
	this->m_Recipients.push_back(idx);
}
inline void CRecipientFilter::RemoveRecipient(const CBasePlayer *player)
{
	if (player == nullptr) return;
	
	auto it = std::find(this->m_Recipients.begin(), this->m_Recipients.end(), ENTINDEX(player));
	if (it == this->m_Recipients.end()) return;
	
	this->m_Recipients.erase(it);
}

inline void CRecipientFilter::AddAllPlayers()
{
	this->m_Recipients.clear();
	
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		CBasePlayer *player = UTIL_PlayerByIndex(i);
		if (player == nullptr) continue;
		
		this->AddRecipient(player);
	}
}
inline void CRecipientFilter::RemoveAllRecipients()
{
	this->m_Recipients.clear();
}

inline void CRecipientFilter::AddRecipientsByPVS(const Vector& origin)
{
	CBitVec<ABSOLUTE_PLAYER_LIMIT> bits;
	engine->Message_DetermineMulticastRecipients(false, origin, bits);
	
	this->AddPlayersFromBitMask(bits);
	
}
inline void CRecipientFilter::RemoveRecipientsByPVS(const Vector& origin)
{
	CBitVec<ABSOLUTE_PLAYER_LIMIT> bits;
	engine->Message_DetermineMulticastRecipients(false, origin, bits);
	
	this->RemovePlayersFromBitMask(bits);
}

inline void CRecipientFilter::AddRecipientsByPAS(const Vector& origin)
{
	CBitVec<ABSOLUTE_PLAYER_LIMIT> bits;
	engine->Message_DetermineMulticastRecipients(true, origin, bits);
	
	this->AddPlayersFromBitMask(bits);
}
inline void CRecipientFilter::RemoveRecipientsByPAS(const Vector& origin)
{
	CBitVec<ABSOLUTE_PLAYER_LIMIT> bits;
	engine->Message_DetermineMulticastRecipients(true, origin, bits);
	
	this->RemovePlayersFromBitMask(bits);
}

//inline void CRecipientFilter::AddRecipientsByTeam(CTeam *team)
//{
//	// TODO
//}
//inline void CRecipientFilter::RemoveRecipientsByTeam(CTeam *team)
//{
//	// TODO
//}

inline void CRecipientFilter::AddPlayersFromBitMask(const CBitVec<ABSOLUTE_PLAYER_LIMIT>& bits)
{
	for (int i = bits.FindNextSetBit(0); i != -1; i = bits.FindNextSetBit(i + 1)) {
		CBasePlayer *player = UTIL_PlayerByIndex(i + 1);
		if (player != nullptr) this->AddRecipient(player);
	}
}
inline void CRecipientFilter::RemovePlayersFromBitMask(const CBitVec<ABSOLUTE_PLAYER_LIMIT>& bits)
{
	for (int i = bits.FindNextSetBit(0); i != -1; i = bits.FindNextSetBit(i + 1)) {
		CBasePlayer *player = UTIL_PlayerByIndex(i + 1);
		if (player != nullptr) this->RemoveRecipient(player);
	}
}


template<bool RELIABLE>
class CSingleUserRecipientFilterT : public CRecipientFilter
{
public:
	CSingleUserRecipientFilterT(const CBasePlayer *player) : CRecipientFilter(RELIABLE)
	{
		this->AddRecipient(player);
	}
};

template<bool RELIABLE>
class CBroadcastRecipientFilterT : public CRecipientFilter
{
public:
	CBroadcastRecipientFilterT() : CRecipientFilter(RELIABLE)
	{
		this->AddAllPlayers();
	}
};

template<bool RELIABLE>
class CPVSFilterT : public CRecipientFilter
{
public:
	CPVSFilterT(const Vector& origin) : CRecipientFilter(RELIABLE)
	{
		this->AddRecipientsByPVS(origin);
	}
};

template<bool RELIABLE>
class CPASFilterT : public CRecipientFilter
{
public:
	CPASFilterT(const Vector& origin) : CRecipientFilter(RELIABLE)
	{
		this->AddRecipientsByPAS(origin);
	}
};


using CSingleUserRecipientFilter = CSingleUserRecipientFilterT<false>;
using CBroadcastRecipientFilter  = CBroadcastRecipientFilterT <false>;
using CPVSFilter                 = CPVSFilterT                <false>;
using CPASFilter                 = CPASFilterT                <false>;

using CReliableSingleUserRecipientFilter = CSingleUserRecipientFilterT<true>;
using CReliableBroadcastRecipientFilter  = CBroadcastRecipientFilterT <true>;
using CReliablePVSFilter                 = CPVSFilterT                <true>;
using CReliablePASFilter                 = CPASFilterT                <true>;


class CUserMessages_Server
{
public:
	int LookupUserMessage(const char *name)   { return ft_LookupUserMessage(this, name); }
	void Register(const char *name, int size) {        ft_Register         (this, name, size); }
	
private:
	static MemberFuncThunk<CUserMessages_Server *, int, const char *>       ft_LookupUserMessage;
	static MemberFuncThunk<CUserMessages_Server *, void, const char *, int> ft_Register;
};


extern GlobalThunk<CUserMessages_Server *> usermessages_sv;


#define CUserMessages CUserMessages_Server
#define usermessages  usermessages_sv


#endif
