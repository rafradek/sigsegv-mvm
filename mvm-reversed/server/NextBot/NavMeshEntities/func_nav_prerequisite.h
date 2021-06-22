/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NavMeshEntities/func_nav_prerequisite.h
 */


// TODO: IFuncNavPrerequisiteAutoList


// sizeof: 0x49c
class CFuncNavPrerequisite : public CBaseTrigger,
	public IFuncNavPrerequisiteAutoList
{
public:
	enum TaskType : int
	{
		// 0 none?
		// 1 destroy?
		MOVE_TO = 2,
		WAIT    = 3,
	};
	
	// TODO
	
private:
	// 484 vtable 2
	TaskType m_task;           // +0x488
	string_t m_taskEntityName; // +0x48c
	// 490 float m_taskValue
	bool m_isDisabled; // +0x494
	// 498 CHandle<T>
};
