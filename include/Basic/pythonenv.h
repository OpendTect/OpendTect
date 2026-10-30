#pragma once
/*+
 _ ____________________*___________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
 ________________________________________________________________________

 -*/

#include "basicmod.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "commontypes.h"
#include "enums.h"
#include "filepath.h"
#include "manobjectset.h"
#include "odprocess.h"
#include "refcount.h"
#include "uistringset.h"

namespace OD
{
    namespace JSON
    {
	class Object;
    }

    mExpClass(Basic) PythonEnv : public CallBacker
    { mODTextTranslationClass(PythonEnv);
    public:
	enum class EnvType {
	    Venv = 0,
	    Conda = 1,
	    Unknown = 2
	};
	mDeclareEnumUtils( EnvType )

				PythonEnv();
				~PythonEnv();

	mStruct(Basic) EnvInfo
	{
	    EnvInfo() {}
	    BufferString	prefix_;	//<! Full prefix path
	    BufferString	interp_;	//<! Full interpreter path
	    BufferString	display_;	//<! Short display string,
	    bool		isdefault_ = false;
	};

	mStruct(Basic) ChildEnvPars
	{
	    BufferStringSet	basepaths_;	//<! Added to PYTHONPATH
	    BufferStringSet	userpaths_;	//<! Added to PYTHONPATH
	    const BufferStringSet* baselinenv_ = nullptr;
	    //<! Start from this environment, or from the process environment
	    BufferString	pathprepend_;	//<! Put in front of PATH
	    BufferString	envprefix_;	/*<! Sets VIRTUAL_ENV or
						    CONDA_PREFIX */
	    bool		isvenv_ = false;
	    bool		cleancustomenv_ = false;
	};

	static BufferStringSet	getChildEnvironment(const ChildEnvPars&);
	/*<! Full environment to hand to a child process. The environment of
	 * the calling process is left untouched. When cleancustomenv_ is set,
	 * the OpendTect installation is removed from PATH and the library
	 * path, the loader and Qt variables are cleared, and DTECT_APPL is
	 * pointed at the current installation. */

	uiRetVal		setEnvironmentRoot(const char*);
	uiRetVal		useJSON(const OD::JSON::Object&,
					bool force=false);
	void			reset();
	int			currentEnvironment() const;
	bool			selectEnvironment(int idx);
	bool			selectEnvironment(const char* prefix);
	EnvType			envType() const;

	EnvInfo			getEnvInfo(int idx) const;
	const ManagedObjectSet<EnvInfo>& getEnvInfos() const;
	int			size() const;
	BufferStringSet		getDisplayNames() const;

	BufferString		pythonPath() const;
	//<! Path of the python executable
	BufferString		pythonBinPath() const;
	//<! Path of the folder containing the python executable
	BufferString		prefixPath() const;
	//<! Path of the environment folder
	BufferString		envRootPath() const;
	//<! Path of the project root
	uiString		statusString() const;
	BufferString		statusText() const;

	BufferStringSet		getRunCommand(const BufferStringSet&) const;
	/*<! Returns a full command (program + arguments) to run in
	 * the current environment. Returns an empty set if the environment is
	 * invalid. */
	BufferStringSet		getShellRunCommand(
						const BufferStringSet&) const;
	/*<! Returns a full command to run a command inside a shell with the
	 * current environment activated, e.g. to launch an interactive
	 * terminal. */
	BufferStringSet		getPythonRunCommand() const;
	/*<! Returns a command prefix to execute the python interpreter of the
	 * current environment. */

	bool			getModuleList(BufferStringSet& modlst,
					      BufferString* errstr=nullptr
					      ) const;
	//<! Get "name version" of all modules in the current environment
	bool			getPythonVersion(BufferString& verstr,
					    BufferString* errstr=nullptr) const;
	//<! Get the python interpreter version of the current environment
	int			runCommand(
				    const BufferStringSet&  runcmd,
				    BufferStringSet* outstrs=nullptr,
				    BufferString* errstr=nullptr,
				    int msecs=30000,
				    const char* workdir=nullptr,
				    const BufferStringSet* environment=nullptr
				) const;
	/*<! Run a command in the current environment. This function blocks
	 * until the the command exits. "msec" is a timeout, use msecs<0 to wait
	 * indefinitely. Returns the command exit code. When environment is
	 * given, it is used as the full environment of the child process. */
	RefMan<Process>		startCommand(
				    const BufferStringSet& runcmd,
				    BufferString* errstr=nullptr,
				    const char* workdir=nullptr,
				    const BufferStringSet* environment=nullptr
				) const;
	/*<! Run a command in the current environment, non-blocking. The
	 * returned process reference can be used to connect to its notifiers or
	 * collect its output and exit code. */
	PID_Type		runCommandBG(
				    const BufferStringSet& runcmd,
				    BufferString* errstr=nullptr,
				    const char* workdir=nullptr,
				    bool createconsole=false,
				    const BufferStringSet* environment=nullptr
				) const;
	/*<! Run a in the current environment, non-blocking. Returns the process
	 * id, or -1 on failure. */

	bool			hasManager() const;
	bool			isCloneable() const;
	BufferStringSet		getCloneCommand(const char* destdir) const;
	//<! Commandline to execute to clone the current environment

	static BufferString	GetPythonPath();
	/*<! PYTHONPATH taking into account OD_USER_PLUGIN_DIR and
	 * OD_APPL_PLUGIN_DIR */

	static void		AddQuotesIfNeeded(BufferStringSet&);

    private:
	BufferString		getEnvBinFolderName() const;
	uiRetVal		resolveEnvironment(const char*);

	BufferString		envroot_;	//<!Path of environment root
	ManagedObjectSet<EnvInfo>	envinfos_;
	int			selenv_		= -1;
	EnvType			envtype_	= EnvType::Unknown;
	BufferString		managernm_;
	BufferString		manager_;
    };
} //namespace OD
