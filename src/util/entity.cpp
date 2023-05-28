#include "stub/tfplayer.h"
#include "util/entity.h"
#include "util/iterate.h"
#include "stub/tfentities.h"

CTFPlayer *SelectRandomReachableEnemy(int team) {
    std::vector<CTFPlayer *> validPlayers;
    ForEachTFPlayer([team, &validPlayers](CTFPlayer *player){
        if (player->GetTeamNumber() != team && player->IsAlive() && PointInRespawnRoom(player, player->WorldSpaceCenter(), false)) {
            validPlayers.push_back(player);
        }
    });

    return !validPlayers.empty() ? validPlayers[RandomInt(0, validPlayers.size()-1)] : nullptr;
}
