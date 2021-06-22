/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_point_weapon_mimic.cpp
 */


CTFPointWeaponMimic::CTFPointWeaponMimic()
{
	this->m_pzsModelOverride = nullptr;
	
	// TODO
}

CTFPointWeaponMimic::~CTFPointWeaponMimic()
{
}


void CTFPointWeaponMimic::Spawn()
{
	// TODO
}


float CTFPointWeaponMimic::GetSpeed() const
{
	return RandomFloat(this->m_flSpeedMin, this->m_flSpeedMax);
}

QAngle CTFPointWeaponMimic::GetFiringAngles() const
{
	// TODO
}


void CTFPointWeaponMimic::Fire()
{
	/* TODO: enum for weapon type */
	switch (this->m_nWeaponType) {
	case 0:
		this->FireRocket();
		break;
	case 1:
		this->FireGrenade();
		break;
	case 2:
		this->FireArrow();
		break;
	case 3:
		this->FireStickyGrenade();
		break;
	}
}

void CTFPointWeaponMimic::FireArrow()
{
	// TODO
}

void CTFPointWeaponMimic::FireGrenade()
{
	// TODO
}

void CTFPointWeaponMimic::FireRocket()
{
	// TODO
}

void CTFPointWeaponMimic::FireStickyGrenade()
{
	// TODO
}
