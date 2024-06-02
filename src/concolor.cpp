#include "concolor.h"
#include "util/misc.h"

#ifndef PLATFORM_64BITS
#include <ANN/ANN.h>
#endif


namespace ColorSpew
{
#if defined POSIX
	enum ColorMode
	{
		CM_DISABLED,
		CM_16_COLORS,
		CM_256_COLORS,
		CM_TRUE_COLOR,
	};
	
	constexpr ColorMode MODE_FILE    = CM_DISABLED;
	constexpr ColorMode MODE_SCREEN  = CM_DISABLED;
	constexpr ColorMode MODE_DEFAULT = CM_TRUE_COLOR;
	
	ColorMode GetColorMode()
	{
		if (!isatty(STDOUT_FILENO)) {
			return MODE_FILE;
		}
		
		const char *TERM = getenv("TERM");
		
		if (TERM != nullptr && strcmp(TERM, "screen") == 0) {
			return MODE_SCREEN;
		}
		
		return MODE_DEFAULT;
	}
	
	
	Vector ConvRGBtoHCL(const Color& c)
	{
		Vector rgb(c.r() / 255.0f, c.g() / 255.0f, c.b() / 255.0f);
		
		float rgb_sum = rgb.x + rgb.y + rgb.z;
		float rgb_min = std::min({rgb.x, rgb.y, rgb.z});
		float rgb_max = std::max({rgb.x, rgb.y, rgb.z});
		
		float chroma    = rgb_max - rgb_min;
		float lightness = (rgb_sum * 2.0f / 3.0f) - 1.0f;
		float hue       = 0.0f;
		
		if (chroma > 0.0f) {
			if (rgb_max == rgb.x) {
				hue = ((rgb.y - rgb.z) / chroma);
			} else if (rgb_max == rgb.y) {
				hue = ((rgb.z - rgb.x) / chroma) + (M_PI * (2.0f / 3.0f));
			} else if (rgb_max == rgb.z) {
				hue = ((rgb.x - rgb.y) / chroma) + (M_PI * (4.0f / 3.0f));
			}
		}
		
		float hue_sin;
		float hue_cos;
		FastSinCos(hue, &hue_sin, &hue_cos);
		
		return Vector(chroma * hue_cos, chroma * hue_sin, lightness);
	}
	
	
	class ColorMap_ANSI16
	{
	public:
		ColorMap_ANSI16()
		{
			/* precompute HCL values */
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x00, 0x00, 0x00))); // normal black
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xaa, 0x00, 0x00))); // normal red
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x00, 0xaa, 0x00))); // normal green
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xff, 0xaa, 0x00))); // normal yellow
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x00, 0x00, 0xaa))); // normal blue
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xaa, 0x00, 0xaa))); // normal magenta
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x00, 0xaa, 0xaa))); // normal cyan
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xaa, 0xaa, 0xaa))); // normal white
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x55, 0x55, 0x55))); // bright black
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xff, 0x55, 0x55))); // bright red
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x55, 0xff, 0x55))); // bright green
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xff, 0xff, 0x55))); // bright yellow
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x55, 0x55, 0xff))); // bright blue
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xff, 0x55, 0xff))); // bright magenta
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0x55, 0xff, 0xff))); // bright cyan
			this->m_Colors.push_back(ConvRGBtoHCL(Color(0xff, 0xff, 0xff))); // bright white
		}
		
		int Map(const Color& rgb)
		{
			Vector hcl = ConvRGBtoHCL(rgb);
			
			int best = -1;
			float best_dist_sqr = FLT_MAX;
			
			for (int i = 0; i < 16; ++i) {
				float dist_sqr = hcl.DistToSqr(this->m_Colors[i]);
				
				if (dist_sqr < best_dist_sqr) {
					best = i;
				}
			}
			
			assert(best != -1);
			return best;
		}
		
	private:
		std::vector<Vector> m_Colors;
	};
	ColorMap_ANSI16 map_ANSI16;
	
	
#ifndef PLATFORM_64BITS
	class ColorMap_ANSI256
	{
	public:
		ColorMap_ANSI256()
		{
			this->m_ColorData = annAllocPts(240, 3);
			
			for (int i = 0; i < 240; ++i) {
				ANNpoint pt = this->m_ColorData[i];
				
				Color rgb;
				if (i < 216) {
					int r = i / 36;
					int g = (i / 6) % 6;
					int b = i % 6;
					
					rgb[0] = (r == 0 ? 0 : 55 + (r * 40));
					rgb[1] = (g == 0 ? 0 : 55 + (r * 40));
					rgb[2] = (b == 0 ? 0 : 55 + (r * 40));
				} else {
					int v = i - 216;
					
					rgb[0] = 8 + (v * 10);
					rgb[1] = 8 + (v * 10);
					rgb[2] = 8 + (v * 10);
				}
				
				Vector hcl = ConvRGBtoHCL(rgb);
				
				pt[0] = hcl.x;
				pt[1] = hcl.y;
				pt[2] = hcl.z;
			}
			
			this->m_Colors = new ANNkd_tree(this->m_ColorData, 240, 3);
		}
		~ColorMap_ANSI256()
		{
			delete this->m_Colors;
			annDeallocPts(this->m_ColorData);
			annClose();
		}
		
		int Map(const Color& rgb)
		{
			Vector hcl = ConvRGBtoHCL(rgb);
			
			float seek[3] = { hcl.x, hcl.y, hcl.z };
			
			ANNidx indexes[1];
			ANNdist dists[1];
			
			this->m_Colors->annkSearch(seek, 1, indexes, dists);
			
			return indexes[0];
		}
		
	private:
		ANNpointArray m_ColorData = nullptr;
		ANNkd_tree *m_Colors      = nullptr;
	};
	ColorMap_ANSI256 map_ANSI256;
#endif
	
	
	SpewRetval_t Spew_Return(SpewType_t type)
	{
		/* can this env variable change during runtime? assuming no here */
		static auto RAISE_ON_ASSERT = getenv("RAISE_ON_ASSERT");
		
		switch (type) {
		case SPEW_ASSERT:
			return (RAISE_ON_ASSERT == nullptr ? SPEW_DEBUGGER : SPEW_CONTINUE);
		case SPEW_ERROR:
			return SPEW_ABORT;
		default:
			return SPEW_CONTINUE;
		}
	}
	
	SpewOutputFunc_t s_SpewOutputBackup = nullptr;

	SpewRetval_t Spew_ANSI_16Colors(SpewType_t type, const char *pMsg)
	{
		int c = map_ANSI16.Map(*GetSpewOutputColor());

		char text[256];
		if (c < 8) {
			snprintf(text, 256, "\e[%dm" "%s" "\e[0m", 30 + c, pMsg);
		} else {
			snprintf(text, 256, "\e[%d;1m" "%s" "\e[0m", 30 + (c - 8), pMsg);
		}
		
		return s_SpewOutputBackup(type, pMsg);
	}
	
#ifndef PLATFORM_64BITS
	SpewRetval_t Spew_ANSI_256Colors(SpewType_t type, const char *pMsg)
	{
		int c = map_ANSI256.Map(*GetSpewOutputColor());
		
		char text[256];
		snprintf(text, 256, "\e[38;2;%dm" "%s" "\e[0m", 16 + c, pMsg);
		
		return s_SpewOutputBackup(type, pMsg);
	}
#endif
	
	SpewRetval_t Spew_ANSI_TrueColor(SpewType_t type, const char *pMsg)
	{
		Color c = *GetSpewOutputColor();
		
		char text[256];
		snprintf(text, 256, "\e[38;2;%d;%d;%dm" "%s" "\e[0m", c.r(), c.g(), c.b(), pMsg);
		
		return s_SpewOutputBackup(type, text);
	}
	
	
	
	
	void Enable()
	{
		if (!engine->IsDedicatedServer()) return;
		
		if (s_SpewOutputBackup == nullptr) {
			s_SpewOutputBackup = GetSpewOutputFunc();
			
			switch (GetColorMode()) {
			case CM_DISABLED:
				break;
			case CM_16_COLORS:
				SpewOutputFunc(&Spew_ANSI_16Colors);
				break;
#ifndef PLATFORM_64BITS
			case CM_256_COLORS:
				SpewOutputFunc(&Spew_ANSI_256Colors);
				break;
#endif
			case CM_TRUE_COLOR:
				SpewOutputFunc(&Spew_ANSI_TrueColor);
				break;
			}
		}
	}
	
	void Disable()
	{
		if (!engine->IsDedicatedServer()) return;
		
		if (s_SpewOutputBackup != nullptr) {
			SpewOutputFunc(s_SpewOutputBackup);
			s_SpewOutputBackup = nullptr;
		}
	}

	ConVar cvar_enable("sig_color_console", "1", FCVAR_NOTIFY,
		"Enable color console output",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			static bool enabled = true;
			bool value = static_cast<ConVar *>(pConVar)->GetBool();
			if (value && !enabled) {
				Enable();
			}
			else if (!value && enabled) {
				Disable();
			}
		});
#else
	void Enable() {}
	void Disable() {}
#endif
}
