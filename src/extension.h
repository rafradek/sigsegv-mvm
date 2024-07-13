#ifndef _INCLUDE_SIGSEGV_EXTENSION_H_
#define _INCLUDE_SIGSEGV_EXTENSION_H_


class CExtSigsegv :
	public SDKExtension,
	public IMetamodListener,
	public IConCommandBaseAccessor,
	public CBaseGameSystemPerFrame
{
public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late) override;
	virtual void SDK_OnUnload() override;
	virtual void SDK_OnAllLoaded() override;
//	virtual void SDK_OnPauseChange(bool paused) override;
	virtual bool QueryRunning(char *error, size_t maxlength) override;
	
#if defined SMEXT_CONF_METAMOD
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late) override;
	virtual bool SDK_OnMetamodUnload(char *error, size_t maxlength) override;
//	virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlength) override;
#endif
	
	virtual bool RegisterConCommandBase(ConCommandBase *pCommand) override;
	IdentityToken_t *GetIdentity() const;

private:
	// CBaseGameSystemPerFrame
	virtual const char *Name() override { return "CExtSigsegv"; }
	virtual void LevelInitPreEntity() override;
	virtual void LevelInitPostEntity() override;
	
	void LoadSoundOverrides();

	IdentityToken_t *identity;
};

extern IPhraseCollection *phrases;
extern IPhraseFile *phrasesFile;
extern CExtSigsegv g_Ext;


#endif
