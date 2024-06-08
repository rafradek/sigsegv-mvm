#include "mod.h"
#include "mod/util/overlay_shared.h"
#include "link/link.h"
#include "stub/usermessages_sv.h"
#include "util/float16.h"
#include "util/scope.h"
#include "util/socket_old.h"


// TODO:
// use -flto in gcc optimize config
// LZ4?
// re-encode colors with fewer bits if possible
// detour the NDebugOverlay implementations that we use in our own mods


// Fixing ENOBUFS in sendto():
// - use a separate thread for packet sending
// - queue packets up for sending
// - in the thread, if we get ENOBUFS, sleep for a bit and then retry


#define FUNC_STATS(dbits) \
	{ \
		static IntervalTimer timer; \
		static int n_calls = 0; \
		static int n_bits  = 0; \
		++n_calls; \
		n_bits += (dbits); \
		if (timer.IsGreaterThen(cvar_stats_interval.GetFloat())) { \
			if (cvar_stats.GetBool()) { \
				DevMsg("%-20s %5.0f calls/sec, %8.3f KB/sec\n", __func__ + 5, \
					(float)n_calls / timer.GetElapsedTime(), \
					(float)n_bits / timer.GetElapsedTime() / (8 * 1024)); \
			} \
			timer.Start(); \
			n_calls = 0; \
			n_bits  = 0; \
		} \
	}


#define MSG_BEGIN() \
	if (cvar_trace.GetBool()) DevMsg("%s\n", __func__); \
	bf_write *msg = sender.Begin(); \
	int dbits = 0; \
	int b1 = msg->GetNumBitsWritten();
#define MSG_END() \
	int b2 = msg->GetNumBitsWritten(); \
	sender.End(); \
	dbits += (b2 - b1); \
	FUNC_STATS(dbits)


namespace Mod::Util::Overlay_Send
{
	ConVar cvar_mtu("sig_util_overlay_send_mtu", "65000", FCVAR_NOTIFY,
		"The maximum size of UDP packets to send through the firehose");
	ConVar cvar_delay("sig_util_overlay_send_delay", "0", FCVAR_NOTIFY,
		"The number of server ticks to delay sending overlay packets to the client");
	
	
	class OverlaySend
	{
	public:
		struct Message
		{
			Message(size_t bits, const uint8_t *src, int delay) :
				bits(bits), delay(delay)
			{
				this->ptr = new uint8_t[BitByte(bits)];
				memcpy(this->ptr, src, BitByte(bits));
			}
			~Message()
			{
				if (this->ptr != nullptr) {
					delete[] this->ptr;
					this->ptr = nullptr;
				}
			}
			
			Message(const Message&) = delete;
			
			size_t bits;
			uint8_t *ptr = nullptr;
			int delay;
		};
		
		bf_write *Begin()
		{
			assert(!this->m_bInProgress);
			this->m_bInProgress = true;
			
			this->m_TempBuf.resize(cvar_mtu.GetInt());
			this->m_Writer.StartWriting(this->m_TempBuf.data(), this->m_TempBuf.size());
			
			return &this->m_Writer;
		}
		
		void End()
		{
			assert(this->m_bInProgress);
			this->m_bInProgress = false;
			
			int bits = this->m_Writer.GetNumBitsWritten();
			this->m_Queue.emplace_back(bits, this->m_TempBuf.data(), cvar_delay.GetInt());
		}
		
		void SendAll()
		{
			this->InitPacket();
			
			for (auto it = this->m_Queue.begin(); it != this->m_Queue.end(); ) {
				auto& msg = *it;
				
				if (msg.delay > 0) {
					--msg.delay;
					++it;
				} else {
					this->SendOne(msg);
					it = this->m_Queue.erase(it);
				}
			}
			
			this->Flush();
		}
		
		void Flush()
		{
			if (this->m_nPacketBits > HeaderBits()) {
				auto bits = reinterpret_cast<uint32_t *>(this->m_Packet.data());
				*bits = this->m_nPacketBits - HeaderBits();
				
				auto tick = reinterpret_cast<int *>(this->m_Packet.data()) + 1;
				*tick = gpGlobals->tickcount;
				
				Firehose_Send(OVERLAY_PORT, BitByte(this->m_nPacketBits), this->m_Packet.data());
			}
			
			this->InitPacket();
		}
		
	private:
		static constexpr int HeaderBytes() { return 8; }
		static constexpr int HeaderBits()  { return 64; }
		
		int DataBytesPerPacket() const
		{
			return cvar_mtu.GetInt() - HeaderBytes();
		}
		
		void InitPacket()
		{
			this->m_Packet.resize(cvar_mtu.GetInt());
			this->m_nPacketBits = HeaderBits();
		}
		
		void SendOne(const Message& msg)
		{
			assert(BitByte(msg.bits) <= this->DataBytesPerPacket());
			
			if (BitByte(this->m_nPacketBits + msg.bits) > this->DataBytesPerPacket()) {
				this->Flush();
			}
			
			bf_write dst(this->m_Packet.data(), this->m_Packet.size());
			dst.SeekToBit(this->m_nPacketBits);
			dst.WriteBits(msg.ptr, msg.bits);
			
			this->m_nPacketBits += msg.bits;
		}
		
		bool m_bInProgress = false;
		std::vector<uint8_t> m_TempBuf;
		bf_write m_Writer;
		
		std::list<Message> m_Queue;
		
		std::vector<uint8_t> m_Packet;
		int m_nPacketBits = 0;
	};
	OverlaySend sender;
	
	
	ConVar cvar_trace("sig_util_overlay_send_trace", "0", FCVAR_NOTIFY,
		"Trace calls to NDebugOverlay functions");
	ConVar cvar_nocon("sig_util_overlay_send_nocon", "0", FCVAR_NOTIFY,
		"Suppress NextBot console output when overlay sending is enabled");
	
	ConVar cvar_stats("sig_util_overlay_send_stats", "0", FCVAR_NOTIFY,
		"Show call and data rate statistics");
	ConVar cvar_stats_interval("sig_util_overlay_send_stats_interval", "1.0", FCVAR_NOTIFY,
		"Interval at which to show statistics");
	
	
#if 0
	int msgid_Overlays = -1;
	
	
	CReliableBroadcastRecipientFilter *filter = nullptr;
	
	RefCount rc_IServerGameDLL_GameFrame;
	DETOUR_DECL_MEMBER(void, IServerGameDLL_GameFrame, bool simulating)
	{
		/* pre-frame */
		if (filter != nullptr) delete filter;
		filter = new CReliableBroadcastRecipientFilter();
		
		SCOPED_INCREMENT(rc_IServerGameDLL_GameFrame);
		DETOUR_MEMBER_CALL(simulating);
		
		/* post-frame */
		// TODO: report stats (bits/sec average based on timer)
	}
	
	
	bf_write *TryStartMessage()
	{
		if (filter == nullptr) {
			filter = new CReliableBroadcastRecipientFilter();
		}
		
		bf_write *msg = engine->UserMessageBegin(filter, msgid_Overlays);
		if (msg == nullptr) {
			DevMsg("UserMessageBegin returned nullptr!\n");
		}
		return msg;
	}
#endif
	
	
	CBasePlayer *GetLocalPlayer()
	{
		static ConVarRef index("sig_util_listenserverhost_index");
		return UTIL_PlayerByIndex(index.GetInt());
	}
	
	
	void AdjustDuration(float& duration)
	{
		if (duration == 0.0f) {
			duration = gpGlobals->interval_per_tick;
		}
	}
	
	
	void Send_Box(const Vector& origin, const Vector& mins, const Vector& maxs, int r, int g, int b, int a, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_BOX, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(origin);
			msg->WriteBitVec3Coord(mins);
			msg->WriteBitVec3Coord(maxs);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_BoxDirection(const Vector& origin, const Vector& mins, const Vector& maxs, const Vector& orientation, int r, int g, int b, int a, float flDuration)
	{
		AdjustDuration(flDuration);
		
		QAngle angles = vec3_angle;
		angles.y = UTIL_VecToYaw(orientation);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_BOX_ANGLES, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(origin);
			msg->WriteBitVec3Coord(mins);
			msg->WriteBitVec3Coord(maxs);
			msg->WriteBitAngles(angles);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_BoxAngles(const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_BOX_ANGLES, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(origin);
			msg->WriteBitVec3Coord(mins);
			msg->WriteBitVec3Coord(maxs);
			msg->WriteBitAngles(angles);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_SweptBox(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_SWEPT_BOX, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(start);
			msg->WriteBitVec3Coord(end);
			msg->WriteBitVec3Coord(mins);
			msg->WriteBitVec3Coord(maxs);
			msg->WriteBitAngles(angles);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_EntityBounds(const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration)
	{
		AdjustDuration(flDuration);
		
		const CCollisionProperty *pCollide = pEntity->CollisionProp();
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_BOX_ANGLES, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(pCollide->GetCollisionOrigin());
			msg->WriteBitVec3Coord(pCollide->OBBMins());
			msg->WriteBitVec3Coord(pCollide->OBBMaxs());
			msg->WriteBitAngles(pCollide->GetCollisionAngles());
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Line(const Vector& origin, const Vector& target, int r, int g, int b, bool noDepthTest, float duration)
	{
		AdjustDuration(duration);
		
		CBasePlayer *player = GetLocalPlayer();
		if (player == nullptr) {
			return;
		}
		
		if (((player->GetAbsOrigin() - origin).LengthSqr() > MAX_OVERLAY_DIST_SQR) &&
			((player->GetAbsOrigin() - target).LengthSqr() > MAX_OVERLAY_DIST_SQR)) {
			return;
		}
		
		Vector clientForward;
		player->EyeVectors(&clientForward);
		
		Vector toOrigin = origin - player->GetAbsOrigin();
		Vector toTarget = target - player->GetAbsOrigin();
		float dotOrigin = DotProduct(clientForward, toOrigin);
		float dotTarget = DotProduct(clientForward, toTarget);
		
		if (dotOrigin < 0.0f && dotTarget < 0.0f) {
			return;
		}
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_LINE, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(origin);
			msg->WriteBitVec3Coord(target);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, duration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Triangle(const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration)
	{
		AdjustDuration(duration);
		
		CBasePlayer *player = GetLocalPlayer();
		if (player == nullptr) {
			return;
		}
		
		Vector to1 = p1 - player->GetAbsOrigin();
		Vector to2 = p2 - player->GetAbsOrigin();
		Vector to3 = p3 - player->GetAbsOrigin();
		
		if ((to1.LengthSqr() > MAX_OVERLAY_DIST_SQR) &&
			(to2.LengthSqr() > MAX_OVERLAY_DIST_SQR) &&
			(to3.LengthSqr() > MAX_OVERLAY_DIST_SQR)) {
			return;
		}
		
		Vector clientForward;
		player->EyeVectors(&clientForward);
		
		float dot1 = DotProduct(clientForward, to1);
		float dot2 = DotProduct(clientForward, to2);
		float dot3 = DotProduct(clientForward, to3);
		
		if (dot1 < 0.0f && dot2 < 0.0f && dot3 < 0.0f) {
			return;
		}
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_TRIANGLE, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(p1);
			msg->WriteBitVec3Coord(p2);
			msg->WriteBitVec3Coord(p3);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, duration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_EntityText(int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_ENTITY_TEXT, OVERLAY_TYPE_BITS);
			msg->WriteUBitLong(entityID, ENTITY_ID_BITS);
			msg->WriteChar(text_offset);
			msg->WriteString(text);
			_float16(&temp, flDuration); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
		MSG_END();
	}
	
	void Send_EntityTextAtPosition(const Vector& origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_ENTITY_TEXT_AT_POSITION, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(origin);
			msg->WriteChar(text_offset);
			msg->WriteString(text);
			_float16(&temp, flDuration); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
		MSG_END();
	}
	
	void Send_Grid(const Vector& vPosition)
	{
		MSG_BEGIN();
			msg->WriteUBitLong(OV_GRID, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(vPosition);
		MSG_END();
	}
	
	void Send_Text(const Vector& origin, const char *text, bool bViewCheck, float flDuration)
	{
		AdjustDuration(flDuration);
		
		CBasePlayer *player = GetLocalPlayer();
		if (player == nullptr) {
			return;
		}
		
		if ((player->GetAbsOrigin() - origin).LengthSqr() > MAX_OVERLAY_DIST_SQR) {
			return;
		}
		
		Vector clientForward;
		player->EyeVectors(&clientForward);
		
		Vector toText = origin - player->GetAbsOrigin();
		float dotPr   = DotProduct(clientForward, toText);
		
		if (dotPr < 0.0f) {
			return;
		}
		
#if 0
		if (bViewCheck) {
			trace_t tr;
			UTIL_TraceLine(player->GetAbsOrigin(), origin, MASK_OPAQUE, nullptr, COLLISION_GROUP_NONE, &tr);
			
			if ((tr.endpos - origin).Length() > 10.0f) {
				return;
			}
		}
#endif
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_TEXT, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(origin);
			msg->WriteString(text);
			msg->WriteOneBit(bViewCheck);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_ScreenText(float flXpos, float flYpos, const char *text, int r, int g, int b, int a, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_SCREEN_TEXT, OVERLAY_TYPE_BITS);
			_float16(&temp, flXpos); msg->WriteWord(temp);
			_float16(&temp, flYpos); msg->WriteWord(temp);
			msg->WriteString(text);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Cross3D_ext(const Vector& position, const Vector& mins, const Vector& maxs, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_CROSS3D_EXT, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			msg->WriteBitVec3Coord(mins);
			msg->WriteBitVec3Coord(maxs);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Cross3D_size(const Vector& position, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_CROSS3D_SIZE, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			_float16(&temp, size); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Cross3DOriented_ang(const Vector& position, const QAngle& angles, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_CROSS3D_ORIENTED_ANG, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			msg->WriteBitAngles(angles);
			_float16(&temp, size); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Cross3DOriented_mat(const matrix3x4_t& m, float size, int c, bool noDepthTest, float flDuration)
	{
		// TODO
	}
	
	void Send_HorzArrow(const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_HORZ_ARROW, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(startPos);
			msg->WriteBitVec3Coord(endPos);
			_float16(&temp, width); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_YawArrow(const Vector& startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_YAW_ARROW, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(startPos);
			_float16(&temp, yaw); msg->WriteWord(temp);
			_float16(&temp, length); msg->WriteWord(temp);
			_float16(&temp, width); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_VertArrow(const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_VERT_ARROW, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(startPos);
			msg->WriteBitVec3Coord(endPos);
			_float16(&temp, width); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Axis(const Vector& position, const QAngle& angles, float size, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_AXIS, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			msg->WriteBitAngles(angles);
			_float16(&temp, size); msg->WriteWord(temp);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Sphere(const Vector& center, float radius, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_SPHERE, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(center);
			_float16(&temp, radius); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Circle(const Vector& position, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		CBasePlayer *player = GetLocalPlayer();
		if (player == nullptr) {
			return;
		}
		
		Vector clientForward;
		player->EyeVectors(&clientForward);
		
		QAngle vecAngles;
		VectorAngles(clientForward, vecAngles);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_CIRCLE_ANG, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			msg->WriteBitAngles(vecAngles);
			_float16(&temp, radius); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(bNoDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Circle_ang(const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_CIRCLE_ANG, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			msg->WriteBitAngles(angles);
			_float16(&temp, radius); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(bNoDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Circle_axes(const Vector& position, const Vector& xAxis, const Vector& yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_CIRCLE_AXES, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			msg->WriteBitVec3Coord(xAxis);
			msg->WriteBitVec3Coord(yAxis);
			_float16(&temp, radius); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(bNoDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Sphere_ang(const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_SPHERE_ANG, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(position);
			msg->WriteBitAngles(angles);
			_float16(&temp, radius); msg->WriteWord(temp);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(bNoDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_Clear()
	{
		MSG_BEGIN();
			msg->WriteUBitLong(OV_CLEAR, OVERLAY_TYPE_BITS);
		MSG_END();
	}
	
	void Send_LineAlpha(const Vector& origin, const Vector& target, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		AdjustDuration(flDuration);
		
		CBasePlayer *player = GetLocalPlayer();
		if (player == nullptr) {
			return;
		}
		
		if (((player->GetAbsOrigin() - origin).LengthSqr() > MAX_OVERLAY_DIST_SQR) &&
			((player->GetAbsOrigin() - target).LengthSqr() > MAX_OVERLAY_DIST_SQR)) {
			return;
		}
		
		Vector clientForward;
		player->EyeVectors(&clientForward);
		
		Vector toOrigin = origin - player->GetAbsOrigin();
		Vector toTarget = target - player->GetAbsOrigin();
		float dotOrigin = DotProduct(clientForward, toOrigin);
		float dotTarget = DotProduct(clientForward, toTarget);
		
		if (dotOrigin < 0.0f && dotTarget < 0.0f) {
			return;
		}
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_LINE_ALPHA, OVERLAY_TYPE_BITS);
			msg->WriteBitVec3Coord(origin);
			msg->WriteBitVec3Coord(target);
			msg->WriteByte(r);
			msg->WriteByte(g);
			msg->WriteByte(b);
			msg->WriteByte(a);
			msg->WriteOneBit(noDepthTest);
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_ScreenRect(float xFrom, float yFrom, float xTo, float yTo, const Color& cFill, const Color& cEdge, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_SCREEN_RECT, OVERLAY_TYPE_BITS);
			_float16(&temp, xFrom); msg->WriteWord(temp);
			_float16(&temp, yFrom); msg->WriteWord(temp);
			_float16(&temp, xTo);   msg->WriteWord(temp);
			_float16(&temp, yTo);   msg->WriteWord(temp);
			msg->WriteLong(cFill.GetRawColor());
			msg->WriteLong(cEdge.GetRawColor());
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_ScreenLine_2color(float xFrom, float yFrom, float xTo, float yTo, const Color& cFrom, const Color& cTo, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_SCREEN_LINE_2COLOR, OVERLAY_TYPE_BITS);
			_float16(&temp, xFrom); msg->WriteWord(temp);
			_float16(&temp, yFrom); msg->WriteWord(temp);
			_float16(&temp, xTo);   msg->WriteWord(temp);
			_float16(&temp, yTo);   msg->WriteWord(temp);
			msg->WriteLong(cFrom.GetRawColor());
			msg->WriteLong(cTo  .GetRawColor());
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	void Send_ScreenLine_1color(float xFrom, float yFrom, float xTo, float yTo, const Color& color, float flDuration)
	{
		AdjustDuration(flDuration);
		
		MSG_BEGIN();
			uint16_t temp;
			msg->WriteUBitLong(OV_SCREEN_LINE_1COLOR, OVERLAY_TYPE_BITS);
			_float16(&temp, xFrom); msg->WriteWord(temp);
			_float16(&temp, yFrom); msg->WriteWord(temp);
			_float16(&temp, xTo);   msg->WriteWord(temp);
			_float16(&temp, yTo);   msg->WriteWord(temp);
			msg->WriteLong(color.GetRawColor());
			_float16(&temp, flDuration); msg->WriteWord(temp);
		MSG_END();
	}
	
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Box, const Vector& origin, const Vector& mins, const Vector& maxs, int r, int g, int b, int a, float flDuration)
	{
		Send_Box(origin, mins, maxs, r, g, b, a, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Box, const Vector& origin, const Vector& mins, const Vector& maxs, int r, int g, int b, int a, float flDuration)
	{
		Send_Box(origin, mins, maxs, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_BoxDirection, const Vector& origin, const Vector& mins, const Vector& maxs, const Vector& orientation, int r, int g, int b, int a, float flDuration)
	{
		Send_BoxDirection(origin, mins, maxs, orientation, r, g, b, a, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_BoxDirection, const Vector& origin, const Vector& mins, const Vector& maxs, const Vector& orientation, int r, int g, int b, int a, float flDuration)
	{
		Send_BoxDirection(origin, mins, maxs, orientation, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_BoxAngles, const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		Send_BoxAngles(origin, mins, maxs, angles, r, g, b, a, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_BoxAngles, const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		Send_BoxAngles(origin, mins, maxs, angles, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_SweptBox, const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		Send_SweptBox(start, end, mins, maxs, angles, r, g, b, a, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_SweptBox, const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		Send_SweptBox(start, end, mins, maxs, angles, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_EntityBounds, const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration)
	{
		Send_EntityBounds(pEntity, r, g, b, a, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_EntityBounds, const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration)
	{
		Send_EntityBounds(pEntity, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Line, const Vector& origin, const Vector& target, int r, int g, int b, bool noDepthTest, float duration)
	{
		Send_Line(origin, target, r, g, b, noDepthTest, duration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Line, const Vector& origin, const Vector& target, int r, int g, int b, bool noDepthTest, float duration)
	{
		Send_Line(origin, target, r, g, b, noDepthTest, duration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Triangle, const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration)
	{
		Send_Triangle(p1, p2, p3, r, g, b, a, noDepthTest, duration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Triangle, const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration)
	{
		Send_Triangle(p1, p2, p3, r, g, b, a, noDepthTest, duration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_EntityText, int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		Send_EntityText(entityID, text_offset, text, flDuration, r, g, b, a);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_EntityText, int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		Send_EntityText(entityID, text_offset, text, flDuration, r, g, b, a);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_EntityTextAtPosition, const Vector& origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		Send_EntityTextAtPosition(origin, text_offset, text, flDuration, r, g, b, a);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_EntityTextAtPosition, const Vector& origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		Send_EntityTextAtPosition(origin, text_offset, text, flDuration, r, g, b, a);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Grid, const Vector& vPosition)
	{
		Send_Grid(vPosition);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Grid, const Vector& vPosition)
	{
		Send_Grid(vPosition);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Text, const Vector& origin, const char *text, bool bViewCheck, float flDuration)
	{
		Send_Text(origin, text, bViewCheck, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Text, const Vector& origin, const char *text, bool bViewCheck, float flDuration)
	{
		Send_Text(origin, text, bViewCheck, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_ScreenText, float flXpos, float flYpos, const char *text, int r, int g, int b, int a, float flDuration)
	{
		Send_ScreenText(flXpos, flYpos, text, r, g, b, a, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_ScreenText, float flXpos, float flYpos, const char *text, int r, int g, int b, int a, float flDuration)
	{
		Send_ScreenText(flXpos, flYpos, text, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3D_ext, const Vector& position, const Vector& mins, const Vector& maxs, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Cross3D_ext(position, mins, maxs, r, g, b, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Cross3D_ext, const Vector& position, const Vector& mins, const Vector& maxs, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Cross3D_ext(position, mins, maxs, r, g, b, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3D_size, const Vector& position, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Cross3D_size(position, size, r, g, b, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Cross3D_size, const Vector& position, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Cross3D_size(position, size, r, g, b, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3DOriented_ang, const Vector& position, const QAngle& angles, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Cross3DOriented_ang(position, angles, size, r, g, b, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Cross3DOriented_ang, const Vector& position, const QAngle& angles, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Cross3DOriented_ang(position, angles, size, r, g, b, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3DOriented_mat, const matrix3x4_t& m, float size, int c, bool noDepthTest, float flDuration)
	{
		Send_Cross3DOriented_mat(m, size, c, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Cross3DOriented_mat, const matrix3x4_t& m, float size, int c, bool noDepthTest, float flDuration)
	{
		Send_Cross3DOriented_mat(m, size, c, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_HorzArrow, const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Send_HorzArrow(startPos, endPos, width, r, g, b, a, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_HorzArrow, const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Send_HorzArrow(startPos, endPos, width, r, g, b, a, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_YawArrow, const Vector& startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Send_YawArrow(startPos, yaw, length, width, r, g, b, a, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_YawArrow, const Vector& startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Send_YawArrow(startPos, yaw, length, width, r, g, b, a, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_VertArrow, const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Send_VertArrow(startPos, endPos, width, r, g, b, a, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_VertArrow, const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Send_VertArrow(startPos, endPos, width, r, g, b, a, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Axis, const Vector& position, const QAngle& angles, float size, bool noDepthTest, float flDuration)
	{
		Send_Axis(position, angles, size, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Axis, const Vector& position, const QAngle& angles, float size, bool noDepthTest, float flDuration)
	{
		Send_Axis(position, angles, size, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Sphere, const Vector& center, float radius, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Sphere(center, radius, r, g, b, noDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Sphere, const Vector& center, float radius, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		Send_Sphere(center, radius, r, g, b, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Circle, const Vector& position, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Circle(position, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Circle, const Vector& position, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Circle(position, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Circle_ang, const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Circle_ang(position, angles, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Circle_ang, const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Circle_ang(position, angles, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Circle_axes, const Vector& position, const Vector& xAxis, const Vector& yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Circle_axes(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Circle_axes, const Vector& position, const Vector& xAxis, const Vector& yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Circle_axes(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Sphere_ang, const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Sphere_ang(position, angles, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Sphere_ang, const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		Send_Sphere_ang(position, angles, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_Clear)
	{
		Send_Clear();
	}
	DETOUR_DECL_STATIC(void, clear_debug_overlays, const CCommand& args)
	{
		DETOUR_STATIC_CALL(args);
		
		Send_Clear();
	}
	
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_LineAlpha, const Vector& origin, const Vector& target, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Send_LineAlpha(origin, target, r, g, b, a, noDepthTest, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_ScreenRect, float xFrom, float yFrom, float xTo, float yTo, const Color& cFill, const Color& cEdge, float flDuration)
	{
		Send_ScreenRect(xFrom, yFrom, xTo, yTo, cFill, cEdge, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_ScreenLine_2color, float xFrom, float yFrom, float xTo, float yTo, const Color& cFrom, const Color& cTo, float flDuration)
	{
		Send_ScreenLine_2color(xFrom, yFrom, xTo, yTo, cFrom, cTo, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, local_NDebugOverlay_ScreenLine_1color, float xFrom, float yFrom, float xTo, float yTo, const Color& color, float flDuration)
	{
		Send_ScreenLine_1color(xFrom, yFrom, xTo, yTo, color, flDuration);
	}
	
	
	RefCount rc_NextBotPlayer_PhysicsSimulate;
	DETOUR_DECL_MEMBER(void, NextBotPlayer_CTFPlayer_PhysicsSimulate)
	{
		SCOPED_INCREMENT(rc_NextBotPlayer_PhysicsSimulate);
		DETOUR_MEMBER_CALL();
	}
	
	RefCount rc_NextBotPlayer_Update;
	DETOUR_DECL_MEMBER(void, NextBotPlayer_CTFPlayer_Update)
	{
		SCOPED_INCREMENT(rc_NextBotPlayer_Update);
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_STATIC(void, ConColorMsg, const Color& clr, const char *pMsg, ...)
	{
		if (cvar_nocon.GetBool() && (rc_NextBotPlayer_PhysicsSimulate > 0 || rc_NextBotPlayer_Update > 0)) {
			return;
		}
		
		static char buf[65536];
		
		va_list args;
		va_start(args, pMsg);
		V_vsprintf_safe(buf, pMsg, args);
		va_end(args);
		
		DETOUR_STATIC_CALL(clr, "%s", buf);
	}
	
	
	RefCount rc_DrawAllDebugOverlays;
	DETOUR_DECL_STATIC(void, DrawAllDebugOverlays)
	{
		SCOPED_INCREMENT(rc_DrawAllDebugOverlays);
		DETOUR_STATIC_CALL();
	}
	
	DETOUR_DECL_MEMBER(bool, IVEngineServer_IsDedicatedServer)
	{
		if (rc_DrawAllDebugOverlays > 0) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL();
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:Overlay_Send")
		{
		//	MOD_ADD_DETOUR_MEMBER(IServerGameDLL_GameFrame, "IServerGameDLL::GameFrame");
			
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Box,                  "NDebugOverlay::Box");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_BoxDirection,         "NDebugOverlay::BoxDirection");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_BoxAngles,            "NDebugOverlay::BoxAngles");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_SweptBox,             "NDebugOverlay::SweptBox");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_EntityBounds,         "NDebugOverlay::EntityBounds");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Line,                 "NDebugOverlay::Line");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Triangle,             "NDebugOverlay::Triangle");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_EntityText,           "NDebugOverlay::EntityText");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_EntityTextAtPosition, "NDebugOverlay::EntityTextAtPosition");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Grid,                 "NDebugOverlay::Grid");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Text,                 "NDebugOverlay::Text");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_ScreenText,           "NDebugOverlay::ScreenText");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3D_ext,          "NDebugOverlay::Cross3D_ext");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3D_size,         "NDebugOverlay::Cross3D_size");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3DOriented_ang,  "NDebugOverlay::Cross3DOriented_ang");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3DOriented_mat,  "NDebugOverlay::Cross3DOriented_mat");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_HorzArrow,            "NDebugOverlay::HorzArrow");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_YawArrow,             "NDebugOverlay::YawArrow");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_VertArrow,            "NDebugOverlay::VertArrow");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Axis,                 "NDebugOverlay::Axis");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Sphere,               "NDebugOverlay::Sphere");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Circle,               "NDebugOverlay::Circle");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Circle_ang,           "NDebugOverlay::Circle_ang");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Circle_axes,          "NDebugOverlay::Circle_axes");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Sphere_ang,           "NDebugOverlay::Sphere_ang");
			
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Box,                  "[local] NDebugOverlay::Box");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_BoxDirection,         "[local] NDebugOverlay::BoxDirection");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_BoxAngles,            "[local] NDebugOverlay::BoxAngles");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_SweptBox,             "[local] NDebugOverlay::SweptBox");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_EntityBounds,         "[local] NDebugOverlay::EntityBounds");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Line,                 "[local] NDebugOverlay::Line");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Triangle,             "[local] NDebugOverlay::Triangle");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_EntityText,           "[local] NDebugOverlay::EntityText");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_EntityTextAtPosition, "[local] NDebugOverlay::EntityTextAtPosition");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Grid,                 "[local] NDebugOverlay::Grid");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Text,                 "[local] NDebugOverlay::Text");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_ScreenText,           "[local] NDebugOverlay::ScreenText");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Cross3D_ext,          "[local] NDebugOverlay::Cross3D_ext");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Cross3D_size,         "[local] NDebugOverlay::Cross3D_size");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Cross3DOriented_ang,  "[local] NDebugOverlay::Cross3DOriented_ang");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Cross3DOriented_mat,  "[local] NDebugOverlay::Cross3DOriented_mat");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_HorzArrow,            "[local] NDebugOverlay::HorzArrow");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_YawArrow,             "[local] NDebugOverlay::YawArrow");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_VertArrow,            "[local] NDebugOverlay::VertArrow");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Axis,                 "[local] NDebugOverlay::Axis");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Sphere,               "[local] NDebugOverlay::Sphere");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Circle,               "[local] NDebugOverlay::Circle");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Circle_ang,           "[local] NDebugOverlay::Circle_ang");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Circle_axes,          "[local] NDebugOverlay::Circle_axes");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Sphere_ang,           "[local] NDebugOverlay::Sphere_ang");
			
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_Clear,                "[local] NDebugOverlay::Clear");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_LineAlpha,            "[local] NDebugOverlay::LineAlpha");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_ScreenRect,           "[local] NDebugOverlay::ScreenRect");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_ScreenLine_2color,    "[local] NDebugOverlay::ScreenLine_2color");
			MOD_ADD_DETOUR_STATIC(local_NDebugOverlay_ScreenLine_1color,    "[local] NDebugOverlay::ScreenLine_1color");
			
			MOD_ADD_DETOUR_STATIC(clear_debug_overlays, "clear_debug_overlays");
			
			MOD_ADD_DETOUR_MEMBER(NextBotPlayer_CTFPlayer_PhysicsSimulate, "NextBotPlayer<CTFPlayer>::PhysicsSimulate");
			MOD_ADD_DETOUR_MEMBER(NextBotPlayer_CTFPlayer_Update,          "NextBotPlayer<CTFPlayer>::Update");
			MOD_ADD_DETOUR_STATIC(ConColorMsg,                             "ConColorMsg");
			
			MOD_ADD_DETOUR_STATIC(DrawAllDebugOverlays,             "DrawAllDebugOverlays");
			MOD_ADD_DETOUR_MEMBER(IVEngineServer_IsDedicatedServer, "IVEngineServer::IsDedicatedServer");
		}
		
#if 0
		virtual bool OnLoad() override
		{
			msgid_Overlays = usermessages->LookupUserMessage("Overlays");
			if (msgid_Overlays == -1) {
				usermessages->Register("Overlays", -1);
				msgid_Overlays = usermessages->LookupUserMessage("Overlays");
				if (msgid_Overlays == -1) {
					DevMsg("Failed to register usermessage \"Overlays\"\n");
					return false;
				}
			}
			
			return true;
		}
#endif
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			sender.SendAll();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_overlay_send", "0", FCVAR_NOTIFY,
		"Utility: overlay forwarding: server send",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_util_overlay_send_bandwidthtest, "")
	{
		int bytes = std::stoi(args[1], nullptr, 0);
		
		while (bytes > 0) {
			int len = Min(bytes, 240);
			
			MSG_BEGIN();
				msg->WriteUBitLong(OV_BANDWIDTH_TEST, OVERLAY_TYPE_BITS);
				msg->WriteWord(len * 8);
				for (int i = 0; i < len; ++i) {
					msg->WriteByte(RandomInt(0x00, 0xff));
				}
			MSG_END();
			
			bytes -= len;
		}
	}
}
