#ifndef pythonaccess_h
#define pythonaccess_h

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
#include "callback.h"
#include "enums.h"

class FilePath;
class uiString;
namespace OS
{
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

	bool		execute(const OS::MachineCommand&) const;
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

	static bool		hasInternalEnvironment(bool allowuserdef=true);
	static bool		validInternalEnvironment(const FilePath&);

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
	FilePath*	activatefp_ = nullptr;
	BufferString	virtenvnm_;
	mutable BufferString	laststdout_;
	mutable BufferString	laststderr_;
	mutable uiString	msg_;

	static bool	getInternalEnvironmentLocation(FilePath&,
							   bool userdef);
	static bool	getSortedVirtualEnvironmentLoc(
				      ObjectSet<FilePath>&,
				      const char* envnm=nullptr,
				      const FilePath* extroot=nullptr);
	bool		isEnvUsable(const FilePath* virtualenvfp,
				    const char* scriptstr,
				    const char* scriptexpectedout);
	static FilePath* getCommand(OS::MachineCommand&,
				      const FilePath* activatefp,
				      const char* envnm);
	static OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
				  const FilePath* activatefp,
				  const char* envnm,
				  FilePath& scriptfp);
	bool			doExecute(const OS::MachineCommand&,
				  const OS::CommandExecPars*,int* pid,
				  const FilePath* activatefp,
				  const char* envnm) const;
	static FilePath*	getActivateScript(const FilePath& root);

    public:

	OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
					 FilePath& scriptfp) const;

    };

    mGlobal(Basic) PythonAccess& PythA();

    mGlobal(Basic) bool canDoCUDA(BufferString& maxverstr);

} //namespace OD

#endif

