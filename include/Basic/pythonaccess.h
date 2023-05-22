#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "bufstring.h"
#include "enums.h"
#include "notify.h"

class FilePath;
class ServiceMgrBase;
class uiPythonSettings;
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

	static BufferString sKeyUseExtPyPath()
				    { return "Use_External_Python_Path"; }

	uiRetVal	isUsable(bool force=false,
				 BufferString* stdoutstr =nullptr,
				 BufferString* stderrstr =nullptr) const;

			/* uiRetVal always contains the output of stderr
			   if the execution failed */
	bool		execute(const OS::MachineCommand&,uiRetVal&,
				bool wait4finish=true,
				BufferString* stdoutstr =nullptr,
				BufferString* stderrstr =nullptr) const;
	bool		execute(const OS::MachineCommand&,
				BufferString& stdoutstr,uiRetVal&,
				BufferString* stderrstr =nullptr) const;
	bool		execute(const OS::MachineCommand&,
				const OS::CommandExecPars&,uiRetVal&,
				int* pid=nullptr,
				BufferString* stdoutstr =nullptr,
				BufferString* stderrstr =nullptr) const;
	bool		executeScript(const char*,BufferString& stdoutstr,
				      uiRetVal&,
				      BufferString* stderrstr =nullptr) const;
	bool		executeScript(const char*,uiRetVal&,bool wait4finish,
				      BufferString* stdoutstr =nullptr,
				      BufferString* stderrstr =nullptr) const;
	bool		executeScript(const BufferStringSet&,
				      BufferString& stdoutstr,uiRetVal&,
				      BufferString* stderrstr =nullptr) const;
	bool		executeScript(const BufferStringSet&,uiRetVal&,
				      bool wait4finish=true,
				      BufferString* stdoutstr =nullptr,
				      BufferString* stderrstr =nullptr) const;

	BufferString	pyVersion() const;
	uiString	pySummary() const;

	bool		isModuleUsable(const char* nm,uiRetVal&) const;

	static BufferString	getDataTypeStr(OD::DataRepType);
	static OD::DataRepType	getDataType(const char*);

	static bool		hasInternalEnvironment(bool allowuserdef=true);
	static bool		validInternalEnvironment(const FilePath&);

	static const char*	sPythonExecNm(bool v3=false,bool v2=false);
	static const char*	sKeyPythonSrc();
	static const char*	sKeyEnviron();
	static const char*	sKeyPythonPath(); //IOPar key only
	static const char*	sKeyActivatePath();

	Notifier<PythonAccess>	envChange;
	CNotifier<PythonAccess,const uiRetVal&> envVerified;

	mStruct(Basic) ModuleInfo : NamedObject
	{
	    public:
				ModuleInfo(const char*);
	    bool		operator ==(const ModuleInfo&) const;
	    bool		operator !=(const ModuleInfo&) const;

	    BufferString	displayStr(bool withver=true) const;

	    BufferString	versionstr_;
	};

	uiRetVal	verifyEnvironment(const char* piname);
			/*<! Non blocking, will trigger envVerified
			     when finished */
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
				  const char* minversion=nullptr) const;
	static uiRetVal	hasModule(const ObjectSet<ModuleInfo>&,
				  const char* modname,
				  const char* minversion=nullptr);
	uiRetVal	getModules(ManagedObjectSet<ModuleInfo>&);
	void		setForScript(const char* scriptnm,
				     OS::MachineCommand&) const;
			/*<! MachineCommand for scripts installed using pip
			     within a conda environment */
	bool		openTerminal(const char* cmd,uiRetVal&,
				     const BufferStringSet* args=nullptr,
				     const char* workdir=nullptr) const;

	uiRetVal	setEnvironment(const FilePath* rootfp,
				       const char* envnm);

	static void	getPathToInternalEnv(FilePath&,bool userdef);
	static void	GetPythonEnvPath(FilePath&);
	static void	GetPythonEnvBinPath(FilePath&);

    private:

	bool		istested_ = false;
	bool		isusable_ = false;
	FilePath*	activatefp_ = nullptr;
	BufferString	virtenvnm_;

	BufferString	pythversion_;
	static BufferStringSet pystartpath_;
	ManagedObjectSet<ModuleInfo> moduleinfos_;

	bool		isUsable_(bool force,uiRetVal&,
				  BufferString* stdoutstr =nullptr,
				  BufferString* stderrstr =nullptr);
	static bool	getInternalEnvironmentLocation(FilePath&,
							   bool userdef);
	static FilePath getInternalEnvPath(bool userdef);
	static bool	getCondaEnvsFromTxt( BufferStringSet& );
	bool		isEnvUsable(const FilePath* pythonenvfp,
				    const char* envnm,uiRetVal&,
				    BufferString* stdoutstr =nullptr,
				    BufferString* stderrstr =nullptr);
	static FilePath* getCommand(OS::MachineCommand&,
				      bool background,
				      const FilePath* activatefp,
				      const char* envnm);
	static OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
				  bool background,
				  const FilePath* activatefp,
				  const char* envnm,
				  FilePath& scriptfp);
	static void		getPIDFromFile(const char* pidfnm,int* pid);
	uiRetVal		doExecute(const OS::MachineCommand&,
				  const OS::CommandExecPars*,int* pid,
				  const FilePath* activatefp,const char* envnm,
				  BufferString* stdoutstr,
				  BufferString* stderrstr) const;
	static FilePath*	getActivateScript(const FilePath& root);
	void			appToBeStartedCB(CallBacker*);
	bool			retrievePythonVersionStr();
	void			verifyEnvironmentCB(CallBacker*);
	const BufferStringSet&	getBasePythonPath_() const;

	void		    createFireWallExeList(ManagedObjectSet<FilePath>&);

	mutable Threads::Lock	lock_;

    public:
			//For specialists only

	PythonSource	getPythonSource() const;
	const FilePath* getActivateFp() const	{ return activatefp_; }
	FilePath	getPythonEnvFp() const;
	const char*	getEnvName() const	{ return virtenvnm_.buf(); }

	static void	initClass();
	static PythonSource getPythonSource(const FilePath* envrootfp);
	static FilePath getPythonEnvFp(const FilePath& activatefp);
	static void	setPythonActivator(const char*);
	static const char* getPythonActivatorPath();
	static bool	needCheckRunScript();

	static bool	getCondaEnvFromTxtPath(ObjectSet<FilePath>&);
	static bool	getSortedVirtualEnvironmentLoc(
				      ObjectSet<FilePath>&,
				      BufferStringSet& envnms,
				      const BufferString* envnm=nullptr,
				      const FilePath* extroot=nullptr);

	void		addBasePath(const FilePath&);
			/*<! For plugins to update PYTHONPATH
			     during initialization */

    };

    mGlobal(Basic) PythonAccess& PythA();

    mGlobal(Basic) bool canDoCUDA(BufferString& maxverstr);

    mGlobal(Basic) uiRetVal pythonRemoveDir(const char* path,
						    bool waitforfin=false);

} // namespace OD
