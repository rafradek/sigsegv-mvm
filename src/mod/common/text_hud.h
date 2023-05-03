#ifndef _INCLUDE_SIGSEGV_MOD_COMMON_TEXT_HUD_H_
#define _INCLUDE_SIGSEGV_MOD_COMMON_TEXT_HUD_H_



inline hudtextparms_t WhiteTextParams(int channel, float x, float y, float holdtime)
{
    hudtextparms_t textparam;
    textparam.channel = channel;
    textparam.x = x;
    textparam.y = y;
    textparam.effect = 0;
    textparam.r1 = 255;
    textparam.r2 = 255;
    textparam.b1 = 255;
    textparam.b2 = 255;
    textparam.g1 = 255;
    textparam.g2 = 255;
    textparam.a1 = 0;
    textparam.a2 = 0; 
    textparam.fadeinTime = 0.f;
    textparam.fadeoutTime = 0.f;
    textparam.holdTime = holdtime;
    textparam.fxTime = 1.0f;
    return textparam;
}

void DisplayHudMessageAutoChannel(CBasePlayer *player, hudtextparms_t & params, const char *message, int internalId);

#endif