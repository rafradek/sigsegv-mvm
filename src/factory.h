#ifndef _INCLUDE_SIGSEGV_FACTORY_H_
#define _INCLUDE_SIGSEGV_FACTORY_H_


inline CreateInterfaceFn EngineFactory()     { return g_SMAPI->GetEngineFactory    (false); }
inline CreateInterfaceFn VPhysicsFactory()   { return g_SMAPI->GetPhysicsFactory   (false); }
inline CreateInterfaceFn FileSystemFactory() { return g_SMAPI->GetFileSystemFactory(false); }
inline CreateInterfaceFn ServerFactory()     { return g_SMAPI->GetServerFactory    (false); }


CreateInterfaceFn CreateInterface();
CreateInterfaceFn ClientFactory();
CreateInterfaceFn SoundEmitterSystemFactory();
CreateInterfaceFn MaterialSystemFactory();
CreateInterfaceFn VGUIFactory();
CreateInterfaceFn VGUIMatSurfaceFactory();
CreateInterfaceFn DedicatedFactory();
CreateInterfaceFn DataCacheFactory();
CreateInterfaceFn VScriptManagerFactory();


#endif
