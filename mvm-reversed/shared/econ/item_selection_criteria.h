/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/econ/item_selection_criteria.h
 */


// sizeof: 0x4c
class CItemSelectionCriteria
{
public:
	class ICondition
	{
		// TODO
	};
	
	class CCondition : public ICondition
	{
		// TODO
	};
	
	class CFloatCondition : public CCondition
	{
		// TODO
	};
	
	class CStringCondition : public CCondition
	{
		// TODO
	};
	
	class CSetCondition : public CCondition
	{
		// TODO
	};
	
	// TODO
	
private:
	bool m_bHasLevel;      // +0x00
	int m_iLevel;          // +0x04
	bool m_bHasQuality;    // +0x08
	int m_iQuality;        // +0x0c
	int m_iInventoryPos;   // +0x10
	bool m_bHasQuantity;   // +0x14
	int m_iQuantity;       // +0x18
	// 1c
	bool m_bIgnoreEnabled; // +0x1d
	// 20 CUtlString
	CUtlVector<int> m_Tags; // +0x24
	CUtlVectorAutoPurge<ICondition *> m_Conditions; // +0x38
};
