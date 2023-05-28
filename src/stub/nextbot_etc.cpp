#include "stub/nextbot_etc.h"

static MemberFuncThunk<NextBotGroundLocomotion *, void, INextBot *> ft_NextBotGroundLocomotion_ctor("NextBotGroundLocomotion::NextBotGroundLocomotion");
NextBotGroundLocomotion *NextBotGroundLocomotion::New(INextBot *bot)
{	
	auto action = reinterpret_cast<NextBotGroundLocomotion *>(::operator new(sizeof(NextBotGroundLocomotion)));
	ft_NextBotGroundLocomotion_ctor(action, bot);
	return action;
}