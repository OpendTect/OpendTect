#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		May 2019
________________________________________________________________________

*/

#include "basicmod.h"

#include "bufstring.h"
#include "enums.h"
#include "notify.h"

class ServiceMgrBase;
class uiPythonSettings;
class uiString;
namespace File {
    class Path;
}
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

	uiRetVal	isUsable(bool force=false,
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
	bool		executeScript(const char*,bool wait4finish=true) const;
	bool		executeScript(const BufferStringSet&,
				      bool wait4finish=true) const;

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
	static const char*	sKeyPythonPath();
	static const char*	sKeyActivatePath();

	Notifier<PythonAccess>	envChange;

	mStruct(Basic) ModuleInfo : NamedObject
	{
	    public:
				ModuleInfo(const char*);

	    BufferString	displayStr(bool withver=true) const;

	    BufferString	versionstr_;
	};

	uiRetVal	verifyEnvironment(const char* piname);
	BufferString	getPacmanExecNm() const;
	uiRetVal	updateModuleInfo(const char* defprog="pip",
					 const char* defarg="list");
			/*<! Pass nullptr to auto-detect */

	void		updatePythonPath() const;
	BufferStringSet getBasePythonPath() const;
			//<! List of dirs containing OpendTect python modules
	BufferStringSet getUserPythonPath() const;
			//<! Initial value of PYTHONPATH before OD starts

	uiRetVal	hasModule(const char* modname,
				  const char* minversion=0) const;
	uiRetVal	getModules(ManagedObjectSet<ModuleInfo>&);
	bool		openTerminal() const;
	static void	getPathToInternalEnv(File::Path&,bool userdef);
	static void	GetPythonEnvPath(File::Path&);
	static void	GetPythonEnvBinPath(File::Path&);

    private:

	bool		istested_ = false;
	bool		isusable_ = false;
	File::Path*	activatefp_ = nullptr;
	BufferString	virtenvnm_;
	mutable PtrMan<OS::CommandLauncher>	cl_;
	mutable BufferString	laststdout_;
	mutable BufferString	laststderr_;
	mutable uiString	msg_;
	BufferString	pythversion_;
	static BufferStringSet pystartpath_;
	ManagedObjectSet<ModuleInfo>			moduleinfos_;

	bool		isUsable_(bool force=false,
				 const char* scriptstr=nullptr,
				 const char* scriptexpectedout=nullptr);
	static bool	getInternalEnvironmentLocation(File::Path&,
							   bool userdef);
	static File::Path getInternalEnvPath(bool userdef);
	static bool	getSortedVirtualEnvironmentLoc(
				      ObjectSet<File::Path>&,
				      BufferStringSet& envnms,
				      const BufferString* envnm=nullptr,
				      const File::Path* extroot=nullptr);
	static bool	getCondaEnvsFromTxt(BufferStringSet&);
	static bool	getCondaEnvFromTxtPath(ObjectSet<File::Path>&);
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
	bool			retrievePythonVersionStr();
	void			envChangeCB(CallBacker*);
	void			pluginsLoaded(CallBacker*);
	const BufferStringSet&	getBasePythonPath_() const;

	friend class ::uiPythonSettings;
	friend class ::ServiceMgrBase;

    public:

	static void	initClass();
	static void	setPythonActivator(const char*);
	static const char* getPythonActivatorPath();
	static bool	needCheckRunScript();

	void		addBasePath(const File::Path&);
			/*<! For plugins to update PYTHONPATH
			     during initialization */

    };

    mGlobal(Basic) PythonAccess& PythA();

    mGlobal(Basic) bool canDoCUDA(BufferString& maxverstr);

    mGlobal(Basic) uiRetVal pythonRemoveDir(const char* path,
						    bool waitforfin=false);

} //namespace OD
