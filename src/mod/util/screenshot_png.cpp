#include "mod.h"
#include "util/scope.h"


namespace Mod::Util::Screenshot_PNG
{
	// detour CL_TakeSnapshotAndSwap
	// - not strictly necessary actually
	
	// detour CVideoMode_Common::TakeSnapshotTGA
	// - refcount (for TGAWriter::WriteToBuffer)
	// - wrap the call, but change pFileName to modify the extension from tga to png
	
	// detour TGAWriter::WriteToBuffer
	// (not a member func, actually a namespace static func)
	// (only when called from CVideoMode_Common::TakeSnapshotTGA)
	// - //////return false so that TakeSnapshotTGA won't attempt to write a TGA file
	// - return actual success value
	// - completely take over and reimplement as a write-PNG-to-CUtlBuffer function
	
	
	DETOUR_DECL_MEMBER(void, CVideoMode_Common_TakeSnapshotTGA, const char *pFilename)
	{
#if defined __GNUC
		#warning TODO
#endif
	}
	
	DETOUR_DECL_STATIC(bool, TGAWriter_WriteToBuffer, unsigned char *pImageData, CUtlBuffer& buffer, int width, int height, ImageFormat srcFormat, ImageFormat dstFormat)
	{
#if defined __GNUC
		#warning TODO
#endif
			return false;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Screenshot_PNG")
		{
			MOD_ADD_DETOUR_MEMBER(CVideoMode_Common_TakeSnapshotTGA, "CVideoMode_Common::TakeSnapshotTGA");
			MOD_ADD_DETOUR_STATIC(TGAWriter_WriteToBuffer,           "TGAWriter::WriteToBuffer");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_screenshot_png", "0", FCVAR_NOTIFY,
		"Utility: instead of writing TGA files for screenshots, write PNG files",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
