#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "pythonaccess.h"

/* Fixtures to create fake python environments. They are header-only because
   every OD_TEST_PROGS entry is built as a separate single-source executable. */

namespace PythonTestFixtures
{

inline bool writeStubFile( const FilePath& fp )
{
    return File::putContent( BufferString("--stub--"), fp.fullPath() );
}


inline bool writeStubExecutable( const FilePath& fp, const char* body=nullptr )
{
    if ( __iswin__ || !body )
	writeStubFile( fp );
    else
	File::putContent( BufferString(body), fp.fullPath() );

    return File::setExecutable( fp.fullPath(), true );
}


/* Writes a fake python interpreter. On Unix it echoes its last argument, or
   dumps a set of environment variables when the last argument is "ENV". This
   lets tests verify that a command line reaches the interpreter unchanged and
   how the child environment is composed. On Windows a plain stub is created. */

inline bool writePythonStub( const FilePath& fp )
{
    if ( __iswin__ )
	return writeStubExecutable( fp );

    const char* script[] =
    {
	"#!/bin/sh\n",
	"last=\n",
	"for a in \"$@\"; do last=\"$a\"; done\n",
	"if [ \"$last\" = \"ENV\" ]; then\n",
	"  printf 'PYTHONPATH=%s\\n' \"$PYTHONPATH\"\n",
	"  printf 'PATH=%s\\n' \"$PATH\"\n",
	"  printf 'VIRTUAL_ENV=%s\\n' \"$VIRTUAL_ENV\"\n",
	"  printf 'CONDA_PREFIX=%s\\n' \"$CONDA_PREFIX\"\n",
	"  printf 'DTECT_APPL=%s\\n' \"$DTECT_APPL\"\n",
	"  printf 'LD_BIND_NOW=%s\\n' \"$LD_BIND_NOW\"\n",
	"  printf 'QT_XCB_GL_INTEGRATION=%s\\n' \"$QT_XCB_GL_INTEGRATION\"\n",
	"  printf 'LC_ALL=%s\\n' \"$LC_ALL\"\n",
	"  printf 'LD_LIBRARY_PATH=%s\\n' \"$LD_LIBRARY_PATH\"\n",
	"  printf 'DYLD_LIBRARY_PATH=%s\\n' \"$DYLD_LIBRARY_PATH\"\n",
	"else\n",
	"  printf '%s\\n' \"$last\"\n",
	"fi\n",
	nullptr
    };

    const BufferStringSet scriptlines( script );
    return File::putContent( scriptlines, fp.fullPath() ) &&
	   File::setExecutable( fp.fullPath(), true );
}


inline FilePath createFakeTool( const FilePath& venvfp,
				const char* toolnm=nullptr )
{
    BufferString nm( toolnm && *toolnm ? toolnm
		     : (__iswin__ ? "odpytest-tool.exe" : "odpytest-tool") );
    const FilePath toolfp( venvfp, __iswin__ ? "Scripts" : "bin", nm );
    writeStubExecutable( toolfp, "#!/bin/sh\nprintf 'VENV_TOOL\\n'\n" );
    return toolfp;
}


/* Creates a fake venv, including a tool executable to test the resolution of
   programs inside the environment. */

inline BufferString createFakeVenv( const FilePath& fp )
{
    if ( !File::exists(fp.fullPath()) )
	File::createDir( fp.fullPath() );

    File::putContent( BufferString("home = /usr\n"),
		      FilePath(fp, "pyvenv.cfg").fullPath() );

    const FilePath binfp( fp, __iswin__ ? "Scripts" : "bin" );
    File::createDir( binfp.fullPath() );
    writePythonStub( FilePath(binfp, OD::PythonAccess::sPythonExecNm()) );
    createFakeTool( fp );

    return fp.fullPath();
}


inline BufferString createFakeUVProject( const FilePath& fp )
{
    if ( !File::exists(fp.fullPath()) )
	File::createDir( fp.fullPath() );

    createFakeVenv( FilePath(fp, ".venv") );
    return fp.fullPath();
}


inline BufferString createFakeCondaEnv( const FilePath& fp )
{
    if ( !File::exists(fp.fullPath()) )
	File::createDir( fp.fullPath() );

    File::createDir( FilePath(fp, "conda-meta").fullPath() );
    FilePath interpfp( fp );
    if ( !__iswin__ )
	interpfp.add( "bin" );
    File::createDir( interpfp.fullPath() );
    writePythonStub( FilePath(interpfp,
			      OD::PythonAccess::sPythonExecNm()) );

    return fp.fullPath();
}


inline BufferString createFakeCondaProject( const FilePath& fp )
{
    createFakeCondaEnv( fp );
    FilePath envsfp = FilePath( fp, "envs" );
    File::createDir( envsfp.fullPath() );
    createFakeCondaEnv( FilePath(envsfp, "first") );
    createFakeCondaEnv( FilePath(envsfp, "second") );
    return fp.fullPath();
}


inline BufferString createFakePixiProject( const FilePath& fp )
{
    if ( !File::exists(fp.fullPath()) )
	File::createDir( fp.fullPath() );

    writeStubFile( FilePath(fp, "pixi.toml") );
    File::createDir( FilePath(fp, ".pixi").fullPath() );
    FilePath envsfp = FilePath( fp, ".pixi", "envs" );
    File::createDir( envsfp.fullPath() );
    createFakeCondaEnv( FilePath(envsfp, "default") );
    createFakeCondaEnv( FilePath(envsfp, "first") );
    createFakeCondaEnv( FilePath(envsfp, "second") );
    return fp.fullPath();
}


inline BufferString createEnvRoot( const char* nm )
{
    const FilePath fp( FilePath::getTempFullPath(nm, nullptr) );
    if ( !File::exists(fp.fullPath()) )
	File::createDir( fp.fullPath() );

    return fp.fullPath();
}


inline void removeFakeEnv( const BufferString& path )
{
    if ( !path.isEmpty() && File::exists(path.buf()) )
	File::removeDir( path.buf() );
}

} // namespace PythonTestFixtures
