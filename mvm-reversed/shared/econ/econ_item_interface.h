/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/econ/econ_item_interface.h
 */


class IEconItemInterface
{
public:
	virtual ~IEconItemInterface();
	
	/* misspelling in original */
	virtual UNKNOWN GetCustomPainkKitDefinition() const;
	virtual UNKNOWN GetCustomPaintKitWear(float& f1) const;
	virtual UNKNOWN IsTradable() const;
	virtual UNKNOWN GetUntradabilityFlags() const;
	virtual UNKNOWN IsCommodity() const;
	virtual UNKNOWN IsUsableInCrafting() const;
	virtual UNKNOWN IsMarketable() const;
	
	virtual UNKNOWN GetItemDefinition() const = 0;
	virtual UNKNOWN GetID() const = 0;
	virtual UNKNOWN GetAccountID() const = 0;
	virtual UNKNOWN GetQuality() const = 0;
	virtual UNKNOWN GetStyle() const = 0;
	virtual UNKNOWN GetFlags() const = 0;
	virtual UNKNOWN GetOrigin() const = 0;
	virtual UNKNOWN GetQuantity() const = 0;
	virtual UNKNOWN GetItemLevel() const = 0;
	virtual UNKNOWN GetInUse() const = 0;
	virtual UNKNOWN GetCustomName() const = 0;
	virtual UNKNOWN GetCustomDesc() const = 0;
	virtual UNKNOWN IterateAttributes(IEconItemAttributeIterator *it) const = 0;
	
	virtual UNKNOWN GetItemDefIndex() const;
	
	// TODO
};
