/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/tf/tf_gamestats_shared.h
 */


enum TFStatType_t
{
	TFSTAT_UNDEFINED          = 0x00,
	TFSTAT_SHOTS_HIT          = 0x01,
	TFSTAT_SHOTS_FIRED        = 0x02,
	TFSTAT_KILLS              = 0x03,
	TFSTAT_DEATHS             = 0x04,
	TFSTAT_DAMAGE             = 0x05,
	TFSTAT_CAPTURES           = 0x06,
	TFSTAT_DEFENSES           = 0x07,
	TFSTAT_DOMINATIONS        = 0x08,
	TFSTAT_REVENGE            = 0x09,
	TFSTAT_POINTSSCORED       = 0x0a,
	TFSTAT_BUILDINGSDESTROYED = 0x0b,
	TFSTAT_HEADSHOTS          = 0x0c,
	TFSTAT_PLAYTIME           = 0x0d,
	TFSTAT_HEALING            = 0x0e,
	TFSTAT_INVULNS            = 0x0f,
	TFSTAT_KILLASSISTS        = 0x10,
	TFSTAT_BACKSTABS          = 0x11,
	TFSTAT_HEALTHLEACHED      = 0x12,
	TFSTAT_BUILDINGSBUILT     = 0x13,
	TFSTAT_MAXSENTRYKILLS     = 0x14,
	TFSTAT_TELEPORTS          = 0x15,
	// 0x16 damage (DMG_IGNITE | DMG_BURN)
	// 0x17 bonus points
	// 0x18 damage (DMG_BLAST)
	// 0x19 damage (MvM bots)
	// 0x1a healthkit pickup
	// 0x1b ammokit pickup
	// 0x1c changed class
	// 0x1d fired weapon?
	// 0x1e suicide
	// 0x1f collected currency
	// 0x20 damage assist
	// 0x21 healed other assist
	// 0x22 boss damage
	// 0x23 blocked damage
	// 0x24 damage (DMG_BLAST_SURFACE)
	// 0x25 damage (DMG_BLAST_SURFACE, not crit boosted)
	// 0x26 damage (DMG_BLAST_SURFACE, crit boosted)
	// 0x27 revive
	// 0x28 throwable hit
	// 0x29 throwable kill
	// 0x2a 
	// 0x2b killed other while under effect of mannpower powerup
	// 0x2c returned flag
	
	TFSTAT_MAX,
}
