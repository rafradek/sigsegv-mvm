/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/entity_currencypack.h
 */


// TODO: ICurrencyPackAutoList
// TODO: IBonusPackAutoList


class CCurrencyPack : public CTFPowerup, public ICurrencyPackAutoList
{
public:
	// TODO
	
private:
	// TODO 0x50a m_bDistributed
};

class CBonusPack : public CCurrencyPack, public IBonusPackAutoList
{
public:
	// TODO
};
