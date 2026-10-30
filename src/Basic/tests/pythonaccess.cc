/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "envvars.h"
#include "dirlist.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"
#include "pythonenv.h"
#include "pythonenvfixtures.h"
#include "settings.h"
#include "string2.h"
#include "typeset.h"

#include <algorithm>
#include <chrono>
#include <cstring>

using namespace PythonTestFixtures;


static BufferString getSetting( const char* key )
{
    BufferString val;
    Settings::fetch( "python" ).get( key, val );
    return val;
}


static void setSetting( const char* key, const char* val )
{
    Settings& settings = Settings::fetch( "python" );
    settings.update( key, val );	// removes when val is empty or null
}


static void restoreEnvVar( const char* key, const BufferString& val )
{
    if ( val.isEmpty() )
	UnsetOSEnvVar( key );
    else
	SetEnvVar( key, val.buf() );
}


static bool testSystemExecuteEnv()
{
    if ( !File::exists(GetODExternalScript()) )
    {
	tstStream(true) << "No external run script, skipping" << od_endl;
	return true;
    }

    const BufferString oldsrc = getSetting( OD::PythonAccess::sKeyPythonSrc() );
    const BufferString oldenv = getSetting( OD::PythonAccess::sKeyEnviron() );
    const BufferString oldforce( GetOSEnvVar("OD_FORCE_PYTHON_ENV_OK") );
    SetEnvVar( "OD_FORCE_PYTHON_ENV_OK", "1" );
    setSetting( OD::PythonAccess::sKeyPythonSrc(), "System" );
    setSetting( OD::PythonAccess::sKeyEnviron(), "" );

    uiRetVal uirv = OD::PythA().isUsable( true );
    mRunStandardTestWithError( uirv.isOK(), "System environment is usable",
			       toString(uirv) );

    const FilePath basefp(
	    FilePath::getTempFullPath("odpytest_sysbase",nullptr) );
    File::createDir( basefp.fullPath() );
    FilePath canonicalbasefp( basefp );
    canonicalbasefp.makeCanonical();
    const BufferString canonicalpath( canonicalbasefp.fullPath() );
    OD::PythA().addBasePath( basefp );

    const BufferString oldpythonpath( GetOSEnvVar("PYTHONPATH") );

    const OS::MachineCommand mc( "/bin/sh", "-c",
				 "printf '%s' \"$PYTHONPATH\"" );
    BufferString stdoutstr, stderrstr;
    uirv.setEmpty();
    const bool res = OD::PythA().execute( mc, stdoutstr, uirv, &stderrstr );
    mRunStandardTestWithError( res && uirv.isOK(), "System command executes",
			       stderrstr.buf() );
    mRunStandardTest( stdoutstr.contains(canonicalpath.buf()),
		      "System child PYTHONPATH contains the base path" );
    mRunStandardTest( BufferString(GetOSEnvVar("PYTHONPATH")) ==
		      oldpythonpath, "System parent PYTHONPATH is unchanged" );

    setSetting( OD::PythonAccess::sKeyPythonSrc(), oldsrc.buf() );
    setSetting( OD::PythonAccess::sKeyEnviron(), oldenv.buf() );
    if ( oldforce.isEmpty() )
	UnsetOSEnvVar( "OD_FORCE_PYTHON_ENV_OK" );
    else
	SetEnvVar( "OD_FORCE_PYTHON_ENV_OK", oldforce.buf() );

    OD::PythA().isUsable( true );
    File::removeDir( basefp.fullPath() );

    return true;
}


static bool testCustomExecuteScript()
{
    const BufferString envpath = createFakeVenv(
	    FilePath(FilePath::getTempFullPath("odpytest_access",nullptr)) );

    const BufferString oldsrc = getSetting( OD::PythonAccess::sKeyPythonSrc() );
    const BufferString oldenv = getSetting( OD::PythonAccess::sKeyEnviron() );
    setSetting( OD::PythonAccess::sKeyPythonSrc(), "Custom" );
    setSetting( OD::PythonAccess::sKeyEnviron(), envpath.buf() );

    uiRetVal uirv = OD::PythA().isUsable( true );
    mRunStandardTestWithError( uirv.isOK(),
			       "Custom environment is usable",
			       toString(uirv) );

    BufferString stdoutstr, stderrstr, script( "print(42)" );
    uirv.setEmpty();
    const bool res = OD::PythA().executeScript( script.buf(), stdoutstr,
						uirv, &stderrstr );
    mRunStandardTestWithError( res && uirv.isOK(),
			       "Custom environment executes a script",
			       stderrstr.buf() );

    //	The stub echoes the -c argument: proves the script reaches the
    //	interpreter and is not swallowed by a nested shell.
    mRunStandardTestWithError( stdoutstr==script,
			       "Script argument reaches the interpreter",
			       BufferString("Got: '",stdoutstr,"'").buf() );

    //	The child must get the OpendTect python paths and the activation
    //	variables, while the parent environment remains untouched.
    const FilePath basefp(
	    FilePath::getTempFullPath("odpytest_base",nullptr) );
    File::createDir( basefp.fullPath() );
    FilePath canonicalbasefp( basefp );
    canonicalbasefp.makeCanonical();
    const BufferString canonicalpath( canonicalbasefp.fullPath() );
    OD::PythA().addBasePath( basefp );

    const FilePath odapplfp(
	    FilePath::getTempFullPath("odpytest_odappl",nullptr) );
    File::createDir( odapplfp.fullPath() );
    const FilePath odbinfp( odapplfp, "bin" );
    File::createDir( odbinfp.fullPath() );
    const FilePath odlibfp( odapplfp, "lib" );
    File::createDir( odlibfp.fullPath() );
    const FilePath syslibfp(
	    FilePath::getTempFullPath("odpytest_syslib",nullptr) );
    File::createDir( syslibfp.fullPath() );

    const BufferString odapplpath( odapplfp.fullPath() );
    const BufferString odbinpath( odbinfp.fullPath() );
    const BufferString odlibpath( odlibfp.fullPath() );
    const BufferString syslibpath( syslibfp.fullPath() );

    const BufferString oldpythonpath( GetOSEnvVar("PYTHONPATH") );
    const BufferString oldpath( GetOSEnvVar("PATH") );
    const BufferString oldcondaprefix( GetOSEnvVar("CONDA_PREFIX") );
    const BufferString olddtectappl( GetOSEnvVar("DTECT_APPL") );
    const BufferString oldldbindnow( GetOSEnvVar("LD_BIND_NOW") );
    const BufferString oldqtxcb( GetOSEnvVar("QT_XCB_GL_INTEGRATION") );
    const BufferString oldlc_all( GetOSEnvVar("LC_ALL") );
    const BufferString oldlang( GetOSEnvVar("LANG") );
#ifdef __mac__
    const char* ldlibkey = "DYLD_LIBRARY_PATH";
#else
    const char* ldlibkey = "LD_LIBRARY_PATH";
#endif
    const BufferString oldldlibpath( GetOSEnvVar(ldlibkey) );
    const BufferString oldsyslibpath(
	    GetOSEnvVar("OD_SYSTEM_LIBRARY_PATH") );

    BufferString newpath( odbinpath );
    if ( !oldpath.isEmpty() )
	newpath.add( ":" ).add( oldpath );
    BufferString newldpath( odlibpath );
    if ( !oldldlibpath.isEmpty() )
	newldpath.add( ":" ).add( oldldlibpath );

    SetEnvVar( "DTECT_APPL", odapplpath.buf() );
    SetEnvVar( "PATH", newpath.buf() );
    SetEnvVar( ldlibkey, newldpath.buf() );
    SetEnvVar( "OD_SYSTEM_LIBRARY_PATH", syslibpath.buf() );
    SetEnvVar( "LD_BIND_NOW", "1" );
    SetEnvVar( "QT_XCB_GL_INTEGRATION", "odpytest_gl" );
    SetEnvVar( "LANG", "C" );
    UnsetOSEnvVar( "LC_ALL" );
    SetEnvVar( "CONDA_PREFIX", "/tmp/odpytest_inherited" );

    BufferString envreport, enverr;
    uirv.setEmpty();
    const bool envres = OD::PythA().executeScript( "ENV", envreport, uirv,
						   &enverr );
    mRunStandardTestWithError( envres && uirv.isOK(),
			       "Custom environment passes its environment",
			       enverr.buf() );
    mRunStandardTest( envreport.contains(canonicalpath.buf()),
		      "Child PYTHONPATH contains the base path" );
    mRunStandardTest( envreport.contains(BufferString("VIRTUAL_ENV=",
						      envpath).buf()),
		      "Child VIRTUAL_ENV is set" );
    mRunStandardTest( !envreport.contains("odpytest_inherited"),
		      "Inherited CONDA_PREFIX is cleared" );
    mRunStandardTest( envreport.contains(BufferString("\nDTECT_APPL=",
						      odapplpath).buf()),
		      "Child DTECT_APPL points at the parent installation" );
    mRunStandardTest( !envreport.contains(odbinpath.buf()),
		      "Child PATH is cleaned of OpendTect paths" );
    mRunStandardTest( envreport.contains(BufferString("\nPATH=",
						      envpath).buf()),
		      "Child PATH is prepended with the env bin" );
    mRunStandardTest( !envreport.contains(odlibpath.buf()),
		      "Child library path is cleaned of OpendTect paths" );
    mRunStandardTest( envreport.contains(syslibpath.buf()),
		      "Child library path contains OD_SYSTEM_LIBRARY_PATH" );
    mRunStandardTest( !envreport.contains("LD_BIND_NOW=1"),
		      "Child LD_BIND_NOW is cleared" );
    mRunStandardTest( !envreport.contains(
			  "QT_XCB_GL_INTEGRATION=odpytest_gl"),
		      "Child QT_XCB_GL_INTEGRATION is cleared" );
    mRunStandardTest( envreport.contains("LC_ALL=C"),
		      "Child LC_ALL is set from LANG" );

    //	A tool from the environment must be resolved inside the environment.
    //	The parent process PATH does not contain the venv bin, so a bare
    //	program name would otherwise be looked up outside the environment.
    OS::MachineCommand toolmc( "odpytest-tool" );
    BufferString toolout, toolerr;
    uirv.setEmpty();
    const bool toolres = OD::PythA().execute( toolmc, toolout, uirv,
					      &toolerr );
    mRunStandardTestWithError( toolres && uirv.isOK(),
			       "Custom environment tool executes",
			       toolerr.buf() );
    mRunStandardTest( toolout.trimBlanks()=="VENV_TOOL",
		      "Custom environment tool comes from the environment" );

    mRunStandardTest( BufferString(GetOSEnvVar("CONDA_PREFIX")) ==
		      BufferString("/tmp/odpytest_inherited"),
		      "Parent CONDA_PREFIX is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar("PYTHONPATH")) ==
		      oldpythonpath, "Parent PYTHONPATH is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar("PATH")) == newpath,
		      "Parent PATH is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar("DTECT_APPL")) == odapplpath,
		      "Parent DTECT_APPL is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar(ldlibkey)) == newldpath,
		      "Parent library path is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar("OD_SYSTEM_LIBRARY_PATH")) ==
		    syslibpath, "Parent OD_SYSTEM_LIBRARY_PATH is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar("LD_BIND_NOW")) == "1",
		      "Parent LD_BIND_NOW is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar("QT_XCB_GL_INTEGRATION")) ==
		      "odpytest_gl",
		      "Parent QT_XCB_GL_INTEGRATION is unchanged" );
    mRunStandardTest( BufferString(GetOSEnvVar("LC_ALL")).isEmpty(),
		      "Parent LC_ALL is unchanged" );

    restoreEnvVar( "CONDA_PREFIX", oldcondaprefix );
    restoreEnvVar( "DTECT_APPL", olddtectappl );
    restoreEnvVar( "PATH", oldpath );
    restoreEnvVar( ldlibkey, oldldlibpath );
    restoreEnvVar( "OD_SYSTEM_LIBRARY_PATH", oldsyslibpath );
    restoreEnvVar( "LD_BIND_NOW", oldldbindnow );
    restoreEnvVar( "QT_XCB_GL_INTEGRATION", oldqtxcb );
    restoreEnvVar( "LANG", oldlang );
    restoreEnvVar( "LC_ALL", oldlc_all );

    File::removeDir( odapplfp.fullPath() );
    File::removeDir( syslibfp.fullPath() );
    File::removeDir( basefp.fullPath() );

    //	Invalid environment must yield an error, not a silent success
    const FilePath invalidfp( FilePath::getTempFullPath("odpytest_missing",
							nullptr) );
    setSetting( OD::PythonAccess::sKeyEnviron(), invalidfp.fullPath().buf() );
    stdoutstr.setEmpty();
    stderrstr.setEmpty();
    uirv.setEmpty();
    const bool invres = OD::PythA().executeScript( script.buf(), stdoutstr,
						   uirv, &stderrstr );
    mRunStandardTest( !invres || !uirv.isOK(),
		      "Invalid custom environment fails" );

    setSetting( OD::PythonAccess::sKeyPythonSrc(), oldsrc.buf() );
    setSetting( OD::PythonAccess::sKeyEnviron(), oldenv.buf() );
    OD::PythA().isUsable( true );
    removeFakeEnv( envpath );

    return true;
}


//---- Execution path benchmark ---------------------------------------------

/* Compares the two PythonAccess execution paths on the same workload:
   doExecute() (internal environment, activated through the activate script)
   and doCustomExecute() (custom environment, through PythonEnv::runCommand).
   Opt-in with --bench, so the regular test run is unaffected. */

namespace
{

const char* sModuleListPayload()
{
    return "import importlib.metadata;"
	   "print("
	   "*[f'{pkg.name} {pkg.version}' "
	   "for pkg in importlib.metadata.distributions() "
	   "if pkg.name not in ['pip','setuptools','wheel']], "
	   "sep='\\n');";
}


struct BenchWorkload
{
    const char*		label_;
    const char*		script_;
};


const int cNrBenchWorkloads = 2;

const BenchWorkload& getBenchWorkload( int idx )
{
    static const BenchWorkload workloads[cNrBenchWorkloads] =
    {
	{ "getModuleList", sModuleListPayload() },
	{ "pass",	   "pass" }
    };

    return workloads[idx];
}


struct BenchSamples
{
    void		add( double msecs )	{ samples_ += msecs; }
    int			size() const		{ return samples_.size(); }
    double		sample( int idx ) const { return samples_[idx]; }

    double		mean() const
			{
			    if ( samples_.isEmpty() )
				return 0;

			    double total = 0;
			    for ( int idx=0; idx<samples_.size(); idx++ )
				total += samples_[idx];

			    return total/samples_.size();
			}

    double		median() const
			{
			    if ( samples_.isEmpty() )
				return 0;

			    TypeSet<double> sorted;
			    sorted = samples_;
			    std::sort( sorted.arr(),
					sorted.arr()+sorted.size() );
			    const int nrsamp = sorted.size();
			    if ( nrsamp%2 )
				return sorted[nrsamp/2];

			    return (sorted[nrsamp/2-1]+sorted[nrsamp/2])/2;
			}

    double		min() const
			{
			    double res = 0;
			    for ( int idx=0; idx<samples_.size(); idx++ )
			    {
				if ( idx==0 || samples_[idx]<res )
				    res = samples_[idx];
			    }

			    return res;
			}

    double		max() const
			{
			    double res = 0;
			    for ( int idx=0; idx<samples_.size(); idx++ )
			    {
				if ( idx==0 || samples_[idx]>res )
				    res = samples_[idx];
			    }

			    return res;
			}

    TypeSet<double>	samples_;
};


struct BenchEnv
{
    BufferString	label_;
    BufferString	workload_;
    BenchSamples	samples_;
    BufferString	out_;
    BufferString	err_;
    bool		ok_ = false;
};


static bool runBenchPayload( const char* script, int nriter, BenchEnv& env )
{
    const OS::MachineCommand mc( OD::PythonAccess::sPythonExecNm(true), "-c",
				 script );
    for ( int idx=0; idx<nriter; idx++ )
    {
	uiRetVal uirv;
	int exitcode = -1;
	BufferString stdoutstr, stderrstr;
	const auto start = std::chrono::steady_clock::now();
	const bool res = OD::PythA().execute( mc, stdoutstr, uirv,
					      &stderrstr, &exitcode );
	const double msecs = std::chrono::duration<double,std::milli>(
			    std::chrono::steady_clock::now()-start).count();
	if ( !res || !uirv.isOK() || exitcode!=0 )
	{
	    env.err_.set( "Execution failed (exit code " ).add( exitcode );
	    if ( !uirv.isOK() )
		env.err_.add( "): " ).add( toString(uirv) );
	    else
		env.err_.add( ")" );

	    if ( !stderrstr.isEmpty() )
		env.err_.add( "\n" ).add( stderrstr );

	    return false;
	}

	env.samples_.add( msecs );
	env.out_ = stdoutstr;
    }

    return true;
}


static void doBench( const BenchWorkload& workload, int reqiters,
			BenchEnv& env )
{
    env.workload_.set( workload.label_ );

//  One warm-up call, also used to size the number of iterations
    BenchEnv warmup;
    if ( !runBenchPayload(workload.script_,1,warmup) )
    {
	env.err_ = warmup.err_;
	return;
    }

    int nriter = reqiters;
    if ( nriter<=0 )
    {
	const double probe = warmup.samples_.mean();
	nriter = probe>0 ? sCast(int,(3000.0/probe)) : 5;
	if ( nriter<5 )
	    nriter = 5;
	else if ( nriter>100 )
	    nriter = 100;
    }

    env.ok_ = runBenchPayload( workload.script_, nriter, env );
}


static BufferStringSet getSortedModules( const BufferString& out )
{
    BufferStringSet res;
    res.unCat( out.buf(), "\n" );
    res.sort();
    return res;
}


static void reportBench( od_ostream& strm, const BenchEnv& env, bool dump )
{
    if ( !env.ok_ )
    {
	strm << env.label_ << od_tab << env.workload_ << od_tab
	     << "FAILED: " << env.err_ << od_endl;
	return;
    }

    strm << env.label_ << od_tab << env.workload_ << od_tab
	 << env.samples_.size()
	 << od_tab << toStringDec( env.samples_.mean(), 2 )
	 << od_tab << toStringDec( env.samples_.median(), 2 )
	 << od_tab << toStringDec( env.samples_.min(), 2 )
	 << od_tab << toStringDec( env.samples_.max(), 2 )
	 << od_tab << getSortedModules(env.out_).size() << od_endl;

    if ( !dump )
	return;

    for ( int idx=0; idx<env.samples_.size(); idx++ )
	strm << "    #" << idx << ": "
	     << toStringDec( env.samples_.sample(idx), 2 ) << od_endl;
}


static BufferString getBenchArgValue( const BufferString& arg, const char* key )
{
    const BufferString keyeq( key, "=" );
    if ( !arg.startsWith(keyeq.buf()) )
	return BufferString::empty();

    return BufferString( arg.buf() + keyeq.size() );
}


static bool setupInternalBench( const FilePath& rootfp,
				const BufferString& envnm,
				BufferString& errmsg )
{
    SetEnvVar( "DTECT_PYTHON_INTERNAL", rootfp.fullPath() );
    setSetting( OD::PythonAccess::sKeyPythonSrc(), "Internal" );
    setSetting( OD::PythonAccess::sKeyEnviron(), envnm.buf() );
    const uiRetVal uirv = OD::PythA().setEnvironment( &rootfp, envnm.buf() );
    if ( uirv.isOK() )
	return true;

    errmsg.set( "Cannot use internal environment: " ).add( toString(uirv) );
    return false;
}


static bool setupCustomBench( const FilePath& envfp, const char* envnm,
			      BufferString& errmsg )
{
    setSetting( OD::PythonAccess::sKeyPythonSrc(), "Custom" );
    setSetting( OD::PythonAccess::sKeyEnviron(), envfp.fullPath() );
    setSetting( sKey::Name(), envnm );
    const uiRetVal uirv = OD::PythA().isUsable( true );
    if ( uirv.isOK() )
	return true;

    errmsg.set( "Cannot use custom environment: " ).add( toString(uirv) );
    return false;
}


static BufferString getCustomEnvReport( const char* label )
{
    const OD::PythonEnv& cenv = OD::PythA().getCustomEnv();
    BufferStringSet cmdline;
    cmdline.add( cenv.pythonPath() );
    BufferString res;
    res.add( label ).add( ":" ).addNewLine()
       .add( "	  type	  : " ).add( cenv.statusText() ).addNewLine()
       .add( "	  prefix  : " ).add( cenv.prefixPath() ).addNewLine()
       .add( "	  manager : " ).add( cenv.hasManager() ? "yes" : "no" )
       .addNewLine()
       .add( "	  runcmd  : " ).add( cenv.getRunCommand(cmdline).cat(" ") );

    return res;
}


static int runPythonAccessBenchmark( int argc, char** argv )
{
    BufferString rootstr( "/opt/seismic/OpendTectPro_rc/Python" );
    BufferString envnm( "odmlpython-cuda113" );
    int reqiters = 0;
    bool dump = false;
    for ( int idx=1; idx<argc; idx++ )
    {
	const BufferString arg( argv[idx] );
	if ( arg=="--bench" )
	    continue;

	BufferString val = getBenchArgValue( arg, "--root" );
	if ( !val.isEmpty() )
	    { rootstr = val; continue; }

	val = getBenchArgValue( arg, "--envnm" );
	if ( !val.isEmpty() )
	    { envnm = val; continue; }

	val = getBenchArgValue( arg, "--iters" );
	if ( !val.isEmpty() )
	    { reqiters = toInt(val); continue; }

	if ( arg=="--dump" )
	    dump = true;
    }

    const FilePath rootfp( rootstr );
    const FilePath envfp( rootfp, "envs", envnm.buf() );
    od_ostream& strm = tstStream();
    if ( !envfp.exists() )
    {
	strm << "Environment does not exist: " << envfp.fullPath() << od_endl;
	return 1;
    }

    strm << od_endl << "PythonAccess execution path benchmark" << od_endl
	 << "  root	   : " << rootfp.fullPath() << od_endl
	 << "  environment : " << envnm << od_endl
	 << "  payloads    : getModuleList (importlib.metadata), pass"
	 << od_endl << od_endl;

    const BufferString oldsrc( getSetting(OD::PythonAccess::sKeyPythonSrc()) );
    const BufferString oldenv( getSetting(OD::PythonAccess::sKeyEnviron()) );
    const BufferString oldname( getSetting(sKey::Name()) );
    const BufferString oldintenv( GetOSEnvVar("DTECT_PYTHON_INTERNAL") );

    const int nrcfgs = 3;
    BenchEnv results[nrcfgs][cNrBenchWorkloads];
    BufferString diag;
    int nrfailed = 0;

    for ( int icfg=0; icfg<nrcfgs; icfg++ )
    {
	BufferString label, errmsg;
	bool setupok = false;
	if ( icfg==0 )
	{
	    label.set( "Internal (doExecute)" );
	    setupok = setupInternalBench( rootfp, envnm, errmsg );
	}
	else if ( icfg==1 )
	{
	    label.set( "Custom (project root)" );
	    setupok = setupCustomBench( rootfp, envnm.buf(), errmsg );
	    if ( setupok )
		diag.add( getCustomEnvReport("custom project root") )
		    .addNewLine();
	}
	else
	{
	    label.set( "Custom (env prefix)" );
	    setupok = setupCustomBench( envfp, nullptr, errmsg );
	    if ( setupok )
		diag.add( getCustomEnvReport("custom env prefix") )
		    .addNewLine();
	}

	for ( int iwork=0; iwork<cNrBenchWorkloads; iwork++ )
	{
	    BenchEnv& env = results[icfg][iwork];
	    env.label_.set( label );
	    if ( !setupok )
	    {
		env.err_ = errmsg;
		nrfailed++;
		continue;
	    }

	    doBench( getBenchWorkload(iwork), reqiters, env );
	    if ( !env.ok_ )
		nrfailed++;
	}
    }

    strm << "config\tworkload\tn\tmean(ms)\tmedian\tmin\tmax\tmodules"
	 << od_endl;
    for ( int icfg=0; icfg<nrcfgs; icfg++ )
    {
	for ( int iwork=0; iwork<cNrBenchWorkloads; iwork++ )
	    reportBench( strm, results[icfg][iwork], dump );
    }

    const BenchEnv& internal = results[0][0];
    const BufferStringSet refmods = getSortedModules( internal.out_ );
    bool allsame = internal.ok_;
    for ( int icfg=1; icfg<nrcfgs; icfg++ )
    {
	const BenchEnv& env = results[icfg][0];
	if ( !env.ok_ )
	    continue;

	allsame = allsame && getSortedModules(env.out_)==refmods;
    }

    if ( internal.ok_ )
    {
	strm << od_endl << "module lists identical : "
	     << (allsame ? "yes" : "no") << od_endl;
	if ( !refmods.isEmpty() )
	{
	    strm << "module count	    : " << refmods.size() << od_endl;
	    strm << "first module	    : " << refmods.get(0) << od_endl;
	    strm << "last module	    : "
		 << refmods.get(refmods.size()-1) << od_endl;
	}
    }

    if ( !diag.isEmpty() )
	strm << od_endl << "resolved custom environments" << od_endl << diag;

    setSetting( OD::PythonAccess::sKeyPythonSrc(), oldsrc.buf() );
    setSetting( OD::PythonAccess::sKeyEnviron(), oldenv.buf() );
    setSetting( sKey::Name(), oldname.buf() );
    restoreEnvVar( "DTECT_PYTHON_INTERNAL", oldintenv );
    OD::PythA().isUsable( true );

    return nrfailed>0 ? 1 : 0;
}

} // anonymous namespace


#define mTestDirNm "Test Dir"

BufferString createTestDir()
{
    const FilePath fp( FilePath::getTempFullPath(mTestDirNm,0) );
    File::createDir( fp.fullPath() );
    const FilePath filefp( fp.fullPath(),
			   FilePath::getTempFileName("python","txt") );
    od_ostream strm( filefp.fullPath() );
    strm << "Testing deletion via python command line";
    strm.close();

    return fp.fullPath();
}


bool testRemoveDir( const char* path, bool expectedres )
{
    const uiRetVal uirv = OD::pythonRemoveDir( path, true );
    BufferString err = toString(uirv);

    const FilePath fp( path );
    const BufferString dirnm = fp.dir();
    const DirList dl( FilePath::getTempDir(), File::DirListType::DirsInDir );
    if ( expectedres )
    {
	bool missing = true;
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    if ( dl.get(idx) == dirnm )
	    {
		missing = false;
		break;
	    }
	}
	if ( !missing )
	    err += "Folder reported deleted but still present";
    }
    else
    {
	bool present = false;
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    if ( dl.get(idx) == dirnm )
	    {
		present = true;
		break;
	    }
	}
	if ( present )
	    err += "Folder to be tested is missing";
    }

    const bool ret = uirv.isOK() && err.isEmpty();
    const BufferString desc = expectedres ? "Delete writable folder"
    : "Don't delete read-only folder";
    mRunStandardTestWithError( ret == expectedres, desc, err );

    return ret;
}

static bool testHasModules()
{
    uiRetVal ret;
    mRunStandardTestWithError( OD::PythA().isModuleUsable( "numpy", ret ),
			       "Has numpy", ret.getText() );
    mRunStandardTestWithError( OD::PythA().isModuleUsable( "pptx", ret ),
			       "Has python-pptx", ret.getText() );
    mRunStandardTestWithError( !OD::PythA().isModuleUsable( "modnotexist", ret),
			"Does not have non-existent module", ret.getText() );

    return true;
}


constexpr int sucessretcode	    = 0;
constexpr int failretcode	    = 1;
constexpr int exceptionretcode	    = 2;

bool testPythonMachineCommand()
{
    FilePath pyscriptfp( __FILE__ );
    pyscriptfp.setFileName( "pythonexitcodetest" ).setExtension( "py" );
    BufferString foundstr( "Python script found" );
    mRunStandardTestWithError( File::exists(pyscriptfp.fullPath()),
			       foundstr.add(": ").add(pyscriptfp.fullPath()),
			       "Python script not found" )

    BufferString stdoutstr, stderrstr;
    uiRetVal uirv;
    OS::MachineCommand mc( OD::PythonAccess::sPythonExecNm(true) );
    mc.addArg( pyscriptfp.fullPath() ).addArg( sucessretcode );

    BufferString teststr( "Running ", mc.toString() );
    teststr.add( ": expected ret code ").add( sucessretcode );
    bool ret = OD::PythA().execute( mc, uirv, true, &stdoutstr, &stderrstr );
    mRunStandardTestWithError( (ret && uirv.isOK()),
			       teststr.buf(), stderrstr.buf() )
    bool isstdoutok = !stdoutstr.isEmpty() &&
    stdoutstr==StringView("Success argument passed");
    mRunStandardTestWithError( isstdoutok,
			       "Valid std output string received",
			       "Wrong std output string" );

    const_cast<BufferStringSet&>(mc.args()).get(1) = failretcode;
    teststr.setEmpty().add( "Running " )
    .add( mc.toString() ).add( ": expected ret code " )
    .add( failretcode );
    stdoutstr.setEmpty();
    stderrstr.setEmpty();
    ret = OD::PythA().execute( mc, uirv, true, &stdoutstr, &stderrstr );
    mRunStandardTestWithError( (!ret ||!uirv.isOK()),
			       teststr.buf(), "Should return false" )
    isstdoutok = !stdoutstr.isEmpty() &&
    stdoutstr==StringView("Error argument passed");
    mRunStandardTestWithError( isstdoutok,
			       "Valid std output string received",
			       "Wrong std output string" );
    const bool isstderrvalid = !stderrstr.isEmpty() &&
    stderrstr==StringView("Fail return");
    mRunStandardTestWithError( isstderrvalid,
			       "Expected error string received",
			       "Expected \"Fail return\" error string" )

    const_cast<BufferStringSet&>(mc.args()).get(1) = exceptionretcode;
    teststr.setEmpty().add( "Running " )
    .add( mc.toString() ).add( ": expected ret code ")
    .add( failretcode );
    stdoutstr.setEmpty();
    stderrstr.setEmpty();
    ret = OD::PythA().execute( mc, uirv, true, &stdoutstr, &stderrstr );
    mRunStandardTestWithError( (!ret || !uirv.isOK()),
			       teststr.buf(), "Should return false" )
    isstdoutok = !stdoutstr.isEmpty() &&
    stdoutstr==StringView("Exception argument passed");
    mRunStandardTestWithError( isstdoutok,
			       "Valid std output string received",
			       "Wrong std output string" );
    mRunStandardTestWithError( !stderrstr.isEmpty(),
			       "Exception raised, received call stack",
			       "Should receive a non-empty exception error" )

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    for ( int idx=1; idx<argc; idx++ )
    {
	if ( BufferString(argv[idx]) == "--bench" )
	    return runPythonAccessBenchmark( argc, argv );
    }

    const uiRetVal uirv = OD::PythA().isUsable();
    if ( !uirv.isOK() )
    {
	tstStream( true ) << toString(uirv) << od_endl;
	return 1;
    }

    const BufferString path = createTestDir();
    File::setReadOnly( path, true );
    if ( testRemoveDir(path,false) )
	return 1;

    File::setWritable( path, true, true );
    if ( !testRemoveDir(path,true) )
	return 1;

    if ( !testHasModules() )
	return 1;

    if ( !testPythonMachineCommand() )
	return 1;

    if ( __iswin__ )
    {
	tstStream( true ) << "No executable stub on Windows, skipping"
			  << od_endl;
	return 0;
    }

    if ( !testSystemExecuteEnv() )
	return 1;

    if ( !testCustomExecuteScript() )
	return 1;

    return 0;
}
