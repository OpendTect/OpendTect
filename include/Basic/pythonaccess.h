#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		May 2019
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"

#include "bufstring.h"
#include "commontypes.h"
#include "enums.h"
#include "filepath.h"
#include "notify.h"

class Timer;
class uiString;
namespace OS {
    class CommandExecPars;
    class CommandLauncher;
    class MachineCommand;
}


namespace OD
{
    enum PythonSource
    {
	Internal, System, Custom
    };
    mDeclareNameSpaceEnumUtils(Basic,PythonSource);

    mExpClass(Basic) PythonAccess : public CallBacker
    { mODTextTranslationClass(PythonAccess);
    public:
			PythonAccess();
			~PythonAccess();

	bool		isUsable(bool force=false,
				 const char* scriptstr=nullptr,
				 const char* scriptexpectedout=nullptr);
	bool		isUsable(bool force=false,
				 const char* scriptstr=nullptr,
				 const char* scriptexpectedout=nullptr) const;

	bool		execute(const OS::MachineCommand&,
				bool wait4finish=true) const;
	bool		execute(const OS::MachineCommand&,
				BufferString& stdoutstr,
				BufferString* stderrstr,
				uiString* errmsg=nullptr) const;
	bool		execute(const OS::MachineCommand&,
				const OS::CommandExecPars&,
				int* pid=nullptr,
				uiString* errmsg=nullptr) const;

	BufferString	lastOutput(bool stderrout,uiString* launchermsg) const;
	BufferString	pyVersion() const;
	uiString	pySummary() const;

	bool		isModuleUsable(const char* nm) const;

	static BufferString	getDataTypeStr(OD::DataRepType);
	static OD::DataRepType	getDataType(const char*);

	static bool		hasInternalEnvironment(bool allowuserdef=true);
	static bool		validInternalEnvironment(const File::Path&);

	static const char*	sPythonExecNm(bool v3=false,bool v2=false);
	static const char*	sKeyPythonSrc();
	static const char*	sKeyEnviron();

	Notifier<PythonAccess>	envChange;

	mStruct(Basic) ModuleInfo : NamedObject
	{
	    public:
				ModuleInfo(const char*);

	    BufferString	displayStr(bool withver=true) const;

	    BufferString	versionstr_;
	};

	uiRetVal	verifyEnvironment(const char* piname);
	uiRetVal	updateModuleInfo(const char* cmd="pip list");
	uiRetVal	hasModule(const char* modname,
				  const char* minversion=0) const;
	uiRetVal	getModules(ManagedObjectSet<ModuleInfo>&);

    private:

	bool		istested_ = false;
	bool		isusable_ = false;
	File::Path*	activatefp_ = nullptr;
	BufferString	virtenvnm_;
	mutable PtrMan<OS::CommandLauncher>	cl_;
	mutable BufferString	laststdout_;
	mutable BufferString	laststderr_;
	mutable uiString	msg_;
	ManagedObjectSet<ModuleInfo>			moduleinfos_;

	static bool	getInternalEnvironmentLocation(File::Path&,
							   bool userdef);
	static File::Path getInternalEnvPath(bool userdef);
	static bool	getSortedVirtualEnvironmentLoc(
				      ObjectSet<File::Path>&,
				      BufferStringSet& envnms,
				      const BufferString* envnm=nullptr,
				      const File::Path* extroot=nullptr);
	bool		isEnvUsable(const File::Path* pythonenvfp,
				    const char* envnm,
				    const char* scriptstr,
				    const char* scriptexpectedout);
	static File::Path* getCommand(OS::MachineCommand&,
				      bool background,
				      const File::Path* activatefp,
				      const char* envnm);
	static OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
				  bool background,
				  const File::Path* activatefp,
				  const char* envnm,
				  File::Path& scriptfp);
	static void		getPIDFromFile(const char* pidfnm,int* pid);
	bool			doExecute(const OS::MachineCommand&,
				  const OS::CommandExecPars*,int* pid,
				  const File::Path* activatefp,
				  const char* envnm) const;
	static File::Path*	getActivateScript(const File::Path& root);

    };

    mGlobal(Basic) PythonAccess& PythA();

    mGlobal(Basic) bool canDoCUDA(BufferString& maxverstr);

} //namespace OD
