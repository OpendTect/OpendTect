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
#include "notify.h"

class Timer;
class uiString;
namespace File { class Path; }
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

	uiRetVal	getModules(ObjectSet<ModuleInfo>&,
				   const char* cmd="pip list");

    private:

	bool		istested_ = false;
	bool		isusable_ = false;
	File::Path*	activatefp_ = nullptr;
	BufferString	virtenvnm_;
	mutable BufferString	laststdout_;
	mutable BufferString	laststderr_;
	mutable uiString	msg_;
	Timer&		filedeltimer_;
	mutable ManagedObjectSet<const File::Path> fptodelset_;

	static bool	getInternalEnvironmentLocation(File::Path&,
							   bool userdef);
	static File::Path getInternalEnvPath(bool userdef);
	static bool	getSortedVirtualEnvironmentLoc(
				      ObjectSet<File::Path>&,
				      const char* envnm=nullptr,
				      const File::Path* extroot=nullptr);
	bool		isEnvUsable(const File::Path* virtualenvfp,
				    const char* scriptstr,
				    const char* scriptexpectedout);
	static File::Path* getCommand(OS::MachineCommand&,
				      const File::Path* activatefp,
				      const char* envnm);
	static OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
				  const File::Path* activatefp,
				  const char* envnm,
				  File::Path& scriptfp);
	bool			doExecute(const OS::MachineCommand&,
				  const OS::CommandExecPars*,int* pid,
				  const File::Path* activatefp,
				  const char* envnm) const;
	static File::Path*	getActivateScript(const File::Path& root);

	void			handleFilesCB(CallBacker*);

    public:

	OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
					 File::Path& scriptfp) const;

    };

    mGlobal(Basic) PythonAccess& PythA();

    mGlobal(Basic) bool canDoCUDA(BufferString& maxverstr);

} //namespace OD
