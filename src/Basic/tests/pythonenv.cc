/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "odjson.h"
#include "pythonaccess.h"
#include "pythonenv.h"
#include "pythonenvfixtures.h"

using namespace PythonTestFixtures;


static bool testInvalidRoot()
{
    OD::PythonEnv penv;
    const uiRetVal uirv = penv.setEnvironmentRoot( "definitely_not_an_env" );
    mRunStandardTestWithError( !uirv.isOK(), "Rejects non-existent root",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Unknown,
		      "Type is Unknown after rejected root" );
    mRunStandardTest( penv.pythonPath().isEmpty(),
		      "Python path cleared after rejected root" );
    mRunStandardTest( penv.envRootPath().isEmpty(),
		      "Env root cleared after rejected root" );

    return true;
}


static bool testNotAnEnv()
{
    const BufferString path = createEnvRoot( "odenvtest_plain" );
    OD::PythonEnv penv;
    const uiRetVal uirv = penv.setEnvironmentRoot( path );
    mRunStandardTestWithError( !uirv.isOK(), "Rejects dir without env marker",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Unknown,
		      "Type stays Unknown" );
    removeFakeEnv( path );

    return true;
}


static bool testInvalidRootAfterValidOne()
{
    const BufferString path = createFakeVenv(
	FilePath(FilePath::getTempFullPath("odenvtest_reset", nullptr)) );

    OD::PythonEnv penv;
    uiRetVal uirv = penv.setEnvironmentRoot( path.buf() );
    mRunStandardTestWithError( uirv.isOK(), "Valid env resolves before reset",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Venv,
		      "Valid env type is Venv" );

    //	A rejected root must not leave the previously resolved environment
    //	looking valid.
    uirv = penv.setEnvironmentRoot( "definitely_not_an_env" );
    mRunStandardTestWithError( !uirv.isOK(), "Rejects invalid second root",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Unknown,
		      "Type is Unknown after rejected second root" );
    mRunStandardTest( penv.pythonPath().isEmpty(),
		      "Python path cleared after rejected second root" );
    mRunStandardTest( penv.envRootPath().isEmpty(),
		      "Env root cleared after rejected second root" );
    mRunStandardTest( penv.size()==0,
		      "Environment list cleared after rejected second root" );

    removeFakeEnv( path );
    return true;
}


static bool testUseJSONWithoutRoot()
{
    const BufferString path = createFakeVenv(
	FilePath(FilePath::getTempFullPath("odenvtest_json", nullptr)) );

    OD::PythonEnv penv;
    uiRetVal uirv = penv.setEnvironmentRoot( path.buf() );
    mRunStandardTestWithError( uirv.isOK(), "Valid env resolves before JSON",
			       toString(uirv) );

    //	A JSON definition without an Environment key must not silently keep
    //	the previously resolved environment.
    OD::JSON::Object jsobj;
    uirv = penv.useJSON( jsobj );
    mRunStandardTestWithError( !uirv.isOK(),
			       "useJSON without Environment errors",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Unknown,
		      "Type is Unknown after empty JSON" );
    mRunStandardTest( penv.envRootPath().isEmpty(),
		      "Env root cleared after empty JSON" );
    mRunStandardTest( penv.size()==0,
		      "Environment list cleared after empty JSON" );

    removeFakeEnv( path );
    return true;
}


static bool testVenvEnv( const char* prefixpath=nullptr )
{
    const BufferString path = prefixpath ? BufferString(prefixpath) :
					   createFakeVenv(
	    FilePath(FilePath::getTempFullPath("odenvtest_venv", nullptr)) );
    FilePath pythonexefp;
    if ( __iswin__ )
	pythonexefp = FilePath( path, "Scripts",
				OD::PythonAccess::sPythonExecNm() );
    else
	pythonexefp = FilePath( path, "bin",
				OD::PythonAccess::sPythonExecNm() );

    OD::PythonEnv penv;
    const uiRetVal uirv = penv.setEnvironmentRoot( path.buf() );
    mRunStandardTestWithError( uirv.isOK(),
			       "Venv environment resolved",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Venv,
		      "Venv type is Venv" );
    mRunStandardTest( !penv.hasManager(), "Venv has no package manager" );
    mRunStandardTest( !penv.isCloneable(), "Venv is not cloneable" );
	mRunStandardTest( penv.prefixPath()==path,
			  "Venv prefix is the env dir" );
    mRunStandardTest( penv.pythonPath()==pythonexefp.fullPath(),
		      "Venv python path is correct" );
    mRunStandardTest( penv.pythonBinPath()==pythonexefp.pathOnly(),
		      "Venv python bin path is correct" );
    if ( !prefixpath )
    {
	mRunStandardTest( penv.envRootPath()==path,
			  "Venv envroot is the venv dir" );
	mRunStandardTest( penv.size()==1, "Venv has only 1 environment" );
	mRunStandardTest( penv.currentEnvironment()==0,
			  "Venv current env is 0");
    }

    const BufferStringSet pythruncmd = penv.getPythonRunCommand();
    mRunStandardTest( pythruncmd.size()==1 &&
		      pythruncmd.get(0)==pythonexefp.fullPath(),
		      "Venv python run command is the venv interpreter" );

    if ( !prefixpath )
	removeFakeEnv( path );
    return true;
}


static bool testCondaEnv(const char* prefixpath=nullptr)
{
    const BufferString path = prefixpath ? BufferString(prefixpath) :
					   createFakeCondaEnv(
	    FilePath(FilePath::getTempFullPath("odenvtest_conda", nullptr)) );
    FilePath pythonexefp;
    if ( __iswin__ )
	pythonexefp = FilePath( path, OD::PythonAccess::sPythonExecNm() );
    else
	pythonexefp = FilePath( path, "bin",
				OD::PythonAccess::sPythonExecNm() );

    OD::PythonEnv penv;
    const uiRetVal uirv = penv.setEnvironmentRoot( path.buf() );
    mRunStandardTestWithError( uirv.isOK(), "Conda environment resolves",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Conda,
		      "Conda type is Conda" );
    mRunStandardTest( penv.prefixPath()==path, "Conda prefix is the env dir" );
    mRunStandardTest( penv.pythonPath()==pythonexefp.fullPath(),
		      "Conda python path is correct" );
    mRunStandardTest( penv.pythonBinPath()==pythonexefp.pathOnly(),
		      "Conda python bin path is correct" );
    if ( !prefixpath )
    {
	mRunStandardTest( penv.envRootPath()==path,
			  "Conda envroot is the env dir" );
	mRunStandardTest( penv.size()==1, "Conda env has only 1 environment" );
	mRunStandardTest( penv.currentEnvironment()==0,
			  "Conda current env is 0")
    }

    if ( !prefixpath )
	removeFakeEnv( path );
    return true;
}


static bool testCondaProject()
{
    const BufferString path = createFakeCondaProject(
	FilePath(FilePath::getTempFullPath("odenvtest_condaproj", nullptr)) );

    OD::PythonEnv penv;
    const uiRetVal uirv = penv.setEnvironmentRoot( path.buf() );
    mRunStandardTestWithError( uirv.isOK(), "Conda Project resolves",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Conda,
		      "Conda Project type is Conda" );
    mRunStandardTest( penv.envRootPath()==path,
		      "Conda Project envroot is the proj dir" );
    mRunStandardTest( penv.size()==3, "Conda Project has 3 environments" );
    mRunStandardTest( penv.currentEnvironment()==0,
		      "Conda Project current env is 0")

    for ( int idx=0; idx<penv.size(); idx++ )
    {
	penv.selectEnvironment( idx );
	mRunStandardTest( testCondaEnv(penv.prefixPath()),
			  "Conda Project child env is valid" );
    }

    removeFakeEnv( path );
    return true;
}


static bool testPixiProject()
{
    const BufferString path = createFakePixiProject(
	FilePath(FilePath::getTempFullPath("odenvtest_pixiproj", nullptr)) );

    OD::PythonEnv penv;
    const uiRetVal uirv = penv.setEnvironmentRoot( path.buf() );
    mRunStandardTestWithError( uirv.isOK(), "Pixi Project resolves",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Conda,
		      "Pixi Project type is Conda" );
    mRunStandardTest( penv.envRootPath()==path,
		      "Pixi Project envroot is the proj dir" );
    mRunStandardTest( penv.size()==3, "Pixi Project has 3 environments" );
    mRunStandardTest( penv.currentEnvironment()==0,
		      "Pixi Project current env is 0")

    for ( int idx=0; idx<penv.size(); idx++ )
    {
	penv.selectEnvironment( idx );
	mRunStandardTest( testCondaEnv(penv.prefixPath()),
			  "Pixi Project child env is valid" );
    }

    removeFakeEnv( path );
    return true;
}


static bool testUVProject()
{
    const BufferString path = createFakeUVProject(
	FilePath(FilePath::getTempFullPath("odenvtest_uvproj", nullptr)) );

    OD::PythonEnv penv;
    const uiRetVal uirv = penv.setEnvironmentRoot( path.buf() );
    mRunStandardTestWithError( uirv.isOK(), "UV Project resolves",
			       toString(uirv) );
    mRunStandardTest( penv.envType()==OD::PythonEnv::EnvType::Venv,
		      "UV Project type is Venv" );
    mRunStandardTest( penv.envRootPath()==path,
		      "UV Project envroot is the proj dir" );
    mRunStandardTest( penv.size()==1, "UV Project has 1 environment" );
    mRunStandardTest( penv.currentEnvironment()==0,
		      "UV Project current env is 0")
    mRunStandardTest( testVenvEnv(penv.prefixPath()),
		      "UV Project child env is valid" );

    removeFakeEnv( path );
    return true;

}


static bool testGetPythonPath()
{
    const BufferString pypath = OD::PythonEnv::GetPythonPath();
    mRunStandardTest( pypath.isEmpty() || pypath.startsWith("PYTHONPATH="),
		      "Python path is empty or PYTHONPATH-prefixed" );

    return true;
}


static bool testVenvCommands()
{
    const BufferString path = createFakeVenv(
	FilePath(FilePath::getTempFullPath("odenvtest_venvcmd", nullptr)) );
    FilePath interpfp;
    if ( __iswin__ )
	interpfp = FilePath( path, "Scripts",
			     OD::PythonAccess::sPythonExecNm() );
    else
	interpfp = FilePath( path, "bin", OD::PythonAccess::sPythonExecNm() );

    OD::PythonEnv penv;
    penv.setEnvironmentRoot( path.buf() );

    BufferStringSet cmdline;
    cmdline.add( interpfp.fullPath() );
    const BufferStringSet runcmd = penv.getRunCommand( cmdline );
    mRunStandardTest( runcmd.size()==1 &&
    runcmd.get(0)==interpfp.fullPath(), "Venv run command constructed" );

    const BufferStringSet pythruncmd = penv.getPythonRunCommand();
    mRunStandardTest( pythruncmd.size()==1 &&
		      pythruncmd.get(0)==interpfp.fullPath(),
		      "Venv python run command prefix constructed" );

    const char* toolnm = __iswin__ ? "odpytest-tool.exe" : "odpytest-tool";
    const FilePath toolfp( __iswin__ ? FilePath(path,"Scripts",toolnm)
				     : FilePath(path,"bin",toolnm) );
    BufferStringSet toolcmd;
    toolcmd.add( toolnm );
    const BufferStringSet toolruncmd = penv.getRunCommand( toolcmd );
    //	Qt and FilePath may use different path separators on Windows.
    mRunStandardTest( toolruncmd.size()==1 &&
		      FilePath(toolruncmd.get(0))==toolfp,
		      "Venv tool resolves inside the environment" );

    const BufferStringSet shellcmd = penv.getShellRunCommand( cmdline );
    mRunStandardTest( !shellcmd.isEmpty() &&
		      (shellcmd.get(0)=="cmd.exe" ||
		       shellcmd.get(0)=="/bin/bash"),
		      "Venv shell command constructed" );

    //	Arguments with spaces must be quoted before the shell re-parse
    BufferStringSet spacedline;
    spacedline.add( interpfp.fullPath() ).add( "/tmp/my script.py" );
    const BufferStringSet spacedshell = penv.getShellRunCommand( spacedline );
    mRunStandardTest( !spacedshell.isEmpty() &&
		      spacedshell.cat().find("/tmp/my script.py") &&
		      spacedshell.cat().find("\"/tmp/my script.py\""),
		      "Venv shell command quotes args with spaces" );

    removeFakeEnv( path );
    return true;
}


static bool testCondaCommands()
{
    const BufferString path = createFakeCondaEnv(
	FilePath(FilePath::getTempFullPath("odenvtest_condacmd", nullptr)) );

    OD::PythonEnv penv;
    penv.setEnvironmentRoot( path.buf() );

    BufferStringSet cmdline;
    cmdline.add( "python" ).add( "-c" ).add( "print(1)" );
    const BufferStringSet runcmd = penv.getRunCommand( cmdline );
    if ( penv.hasManager() )
    {
	mRunStandardTest( penv.isCloneable(),
			  "Conda with manager is cloneable" );
	//	Windows inserts an extra -r <tmp> before the run command
	const int nrextra = __iswin__ ? 6 : 4;
	const int runidx = runcmd.indexOf( "run" );
	mRunStandardTest( runcmd.size()==cmdline.size()+nrextra &&
			  runidx>0 &&
			  runcmd.get(runidx+1)=="-p" &&
			  runcmd.get(runidx+2)==penv.prefixPath() &&
			  runcmd.get(runidx+3)=="python",
			  "Conda with manager runs via the manager" );
    }
    else
    {
	mRunStandardTest( !penv.isCloneable(),
			  "Conda without manager is not cloneable" );
	const BufferString runprog( runcmd.isEmpty()
				  ? BufferString()
			: BufferString(FilePath(runcmd.get(0)).fileName()) );
	mRunStandardTest( runcmd.size()==cmdline.size() &&
			  runprog==OD::PythonAccess::sPythonExecNm(),
			  "Conda without manager runs directly" );
	const BufferStringSet shellcmd = penv.getShellRunCommand( cmdline );
	mRunStandardTest( !shellcmd.isEmpty() &&
			  (shellcmd.get(0)=="cmd.exe" ||
			   shellcmd.get(0)=="/bin/bash"),
			  "Conda without manager has shell wrapper" );
    }

    removeFakeEnv( path );
    return true;
}


static bool testInvalidCommands()
{
    OD::PythonEnv penv;
    BufferStringSet cmdline;
    cmdline.add( "python" ).add( "-c" ).add( "print(1)" );
    mRunStandardTest( penv.getRunCommand(cmdline).isEmpty(),
		      "Invalid environment yields empty run command" );
    mRunStandardTest( penv.getShellRunCommand(cmdline).isEmpty(),
		      "Invalid environment yields empty shell command" );
    mRunStandardTest( !penv.hasManager(),
		      "Invalid environment has no manager" );
    mRunStandardTest( !penv.isCloneable(),
		      "Invalid environment not cloneable" );

    return true;
}


#ifdef __unix__
static bool testChildEnvironment()
{
    const BufferString olddtectappl( GetOSEnvVar("DTECT_APPL") );
    const BufferString odappl(
	FilePath::getTempFullPath("odenvtest_childenv_od", nullptr) );
    const BufferString odbin( FilePath(odappl,"bin").fullPath() );
    const BufferString odlib( FilePath(odappl,"lib").fullPath() );
    const BufferString otherdir(
	FilePath::getTempFullPath("odenvtest_childenv_other", nullptr) );
    File::createDir( odappl );
    File::createDir( odbin );
    File::createDir( odlib );
    File::createDir( otherdir );

    SetEnvVar( "DTECT_APPL", odappl.buf() );

    BufferStringSet baseline;
    baseline.add( BufferString("PATH=",odbin).add(":/usr/bin") );
    baseline.add( BufferString("LD_LIBRARY_PATH=",odlib)
				     .add(":").add(otherdir) );
    baseline.add( "LD_BIND_NOW=1" );

    //	Without cleaning, the inherited environment is passed on as-is
    OD::PythonEnv::ChildEnvPars pars;
    pars.baselinenv_ = &baseline;
    const BufferString inherited(
	OD::PythonEnv::getChildEnvironment(pars).cat("\n") );
    mRunStandardTest( inherited.contains(odlib.buf()),
		      "Uncleaned child keeps the inherited lib path" );
    mRunStandardTest( inherited.contains("LD_BIND_NOW=1"),
		      "Uncleaned child keeps the loader variable" );

    //	With cleaning, the OpendTect installation is removed
    pars.cleancustomenv_ = true;
    const BufferString cleaned(
	OD::PythonEnv::getChildEnvironment(pars).cat("\n") );
    mRunStandardTest( !cleaned.contains(odbin.buf()) &&
		      !cleaned.contains(odlib.buf()),
		      "Cleaned child lacks the OpendTect paths" );
    mRunStandardTest( cleaned.contains(otherdir.buf()),
		      "Cleaned child keeps the system lib path" );
    mRunStandardTest( cleaned.contains("PATH=/usr/bin"),
		      "Cleaned child keeps the system PATH" );
    mRunStandardTest( !cleaned.contains("LD_BIND_NOW=1"),
		      "Cleaned child has no loader variable" );
    mRunStandardTest( cleaned.contains(
			BufferString("DTECT_APPL=",odappl).buf()),
		      "Cleaned child points DTECT_APPL at the installation" );

    //	The python environment and activation variables are applied
    pars.basepaths_.add( "/odpytest/base" );
    pars.userpaths_.add( "/odpytest/user" );
    pars.pathprepend_.set( "/odpytest/env/bin" );
    pars.envprefix_.set( "/odpytest/env" );
    pars.isvenv_ = true;
    const BufferString composed(
	OD::PythonEnv::getChildEnvironment(pars).cat("\n") );
    mRunStandardTest( composed.contains(
		BufferString("PYTHONPATH=/odpytest/base:/odpytest/user").buf()),
		      "Child PYTHONPATH has the base and user paths" );
    mRunStandardTest( composed.contains("PATH=/odpytest/env/bin:"),
		      "Child PATH is prepended with the env bin" );
    mRunStandardTest( composed.contains("VIRTUAL_ENV=/odpytest/env") &&
		      !composed.contains("CONDA_PREFIX=/odpytest/env"),
		      "Child is marked as a venv" );
    pars.isvenv_ = false;
    const BufferString conda(
	OD::PythonEnv::getChildEnvironment(pars).cat("\n") );
    mRunStandardTest( conda.contains("CONDA_PREFIX=/odpytest/env") &&
		      !conda.contains("VIRTUAL_ENV=/odpytest/env"),
		      "Child is marked as a conda environment" );

    SetEnvVar( "DTECT_APPL", olddtectappl.buf() );
    removeFakeEnv( odappl );
    removeFakeEnv( otherdir );
    return true;
}


static bool testRunCommandChildEnv()
{
    const char* ldlibkey = "LD_LIBRARY_PATH";
    const BufferString oldldlib( GetOSEnvVar(ldlibkey) );
    const BufferString olddtectappl( GetOSEnvVar("DTECT_APPL") );

    const BufferString odappl(
	FilePath::getTempFullPath("odenvtest_runenv_od", nullptr) );
    const BufferString odlib( FilePath(odappl,"lib").fullPath() );
    const BufferString otherdir(
	FilePath::getTempFullPath("odenvtest_runenv_other", nullptr) );
    File::createDir( odappl );
    File::createDir( odlib );
    File::createDir( otherdir );
    SetEnvVar( "DTECT_APPL", odappl.buf() );
    const BufferString runldlib( BufferString(odlib,":",otherdir) );
    SetEnvVar( ldlibkey, runldlib.buf() );

    const BufferString path = createFakeVenv(
	FilePath(FilePath::getTempFullPath("odenvtest_runenv", nullptr)) );

    OD::PythonEnv penv;
    penv.setEnvironmentRoot( path.buf() );

    BufferStringSet cmdline;
    cmdline.add( penv.pythonPath() ).add( "ENV" );
    BufferStringSet outstrs;
    BufferString errstr;
    const int res = penv.runCommand( cmdline, &outstrs, &errstr );
    const BufferString envreport( outstrs.cat("\n") );

    mRunStandardTestWithError( res==0, "Child process ran", errstr.buf() );
    mRunStandardTest( envreport.contains(otherdir.buf()),
		      "Child library path keeps the system paths" );
    mRunStandardTest( !envreport.contains(odlib.buf()),
		      "Child library path excludes the OpendTect paths" );
    mRunStandardTest( BufferString(GetOSEnvVar(ldlibkey))==runldlib,
		      "Parent library path is unchanged" );

    if ( oldldlib.isEmpty() )
	UnsetOSEnvVar( ldlibkey );
    else
	SetEnvVar( ldlibkey, oldldlib.buf() );
    SetEnvVar( "DTECT_APPL", olddtectappl.buf() );
    removeFakeEnv( path );
    removeFakeEnv( odappl );
    removeFakeEnv( otherdir );
    return true;
}
#endif


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testInvalidRoot() )
	return 1;
    if ( !testNotAnEnv() )
	return 1;
    if ( !testInvalidRootAfterValidOne() )
	return 1;
    if ( !testUseJSONWithoutRoot() )
	return 1;
    if ( !testVenvEnv() )
	return 1;
    if ( !testCondaEnv() )
	return 1;
    if ( !testCondaProject() )
	return 1;
    if ( !testPixiProject() )
	return 1;
    if ( !testUVProject() )
	return 1;
    if ( !testGetPythonPath() )
	return 1;
    if ( !testVenvCommands() )
	return 1;
    if ( !testCondaCommands() )
	return 1;
    if ( !testInvalidCommands() )
	return 1;
#ifdef __unix__
    if ( !testChildEnvironment() )
	return 1;
    if ( !testRunCommandChildEnv() )
	return 1;
#endif

    return 0;
}
