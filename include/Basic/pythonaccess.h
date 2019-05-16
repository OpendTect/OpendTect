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
#include "enums.h"

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

    mExpClass(Basic) PythonAccess
    { mODTextTranslationClass(PythonAccess);
    public:
			~PythonAccess();

	bool		isUsable(bool force=false);

	bool		execute(const OS::MachineCommand&,
				BufferString& stdoutstr,
				BufferString* stderrstr,
				uiString* errmsg=nullptr);
	bool		execute(const OS::MachineCommand&,
				const OS::CommandExecPars&,
				int* pid=nullptr,uiString* errmsg=nullptr);

	static bool		hasInternalEnvironment(bool allowuserdef=true);
	static bool		validInternalEnvironment(const File::Path&);

	static const char*	sPythonExecNm(bool v3=false,bool v2=false);
	static const char*	sKeyPythonSrc();
	static const char*	sKeyEnviron();

	mStruct(Basic) ModuleInfo : NamedObject
	{
	    public:
			    ModuleInfo(const char*);

	    BufferString	versionstr_;
	};

    private:

	bool		istested_ = false;
	bool		isusable_ = false;
	File::Path*	activatefp_ = nullptr;
	BufferString	virtenvnm_;

	static bool	getInternalEnvironmentLocation(File::Path&,
							   bool userdef);
	static bool	getSortedVirtualEnvironmentLoc(
				      ObjectSet<File::Path>&,
				      const char* envnm=nullptr,
				      const File::Path* extroot=nullptr);
	bool		isEnvUsable(const File::Path* virtualenvfp,
					bool tensorflowtest=false);
	static File::Path* getCommand(OS::MachineCommand&,
				      const File::Path* activatefp,
				      const char* envnm);
	static OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
				  const File::Path* activatefp,
				  const char* envnm,
				  File::Path& scriptfp);
	static bool	doExecute(const OS::MachineCommand&,
				  const OS::CommandExecPars*,int* pid,
				  BufferString* stdoutstr,
				  BufferString* stderrstr,
				  const File::Path* activatefp,
				  const char* envnm,uiString* errmsg);
	static File::Path*	getActivateScript(const File::Path& root);

    public:

	OS::CommandLauncher* getLauncher(const OS::MachineCommand&,
				     File::Path& scriptfp);

    };

    mGlobal(Basic) PythonAccess& PythA();

} //namespace OD
