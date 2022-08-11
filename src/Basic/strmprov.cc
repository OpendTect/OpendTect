/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/


#include "strmprov.h"

#include "datapack.h"
#include "keystrs.h"
#include "iopar.h"
#include "envvars.h"
#include "filesystemaccess.h"
#include "oscommand.h"
#include "perthreadrepos.h"
#include "strmdata.h"
#include "uistrings.h"
#include <iostream>


#ifndef OD_NO_QT
# include "qstreambuf.h"
# include <QProcess>
#endif

#include "file.h"
#include "filepath.h"
#include "string2.h"
#include "namedobj.h"
#include "debug.h"
#include "executor.h"
#include "fixedstreambuf.h"


const char* StreamProvider::sStdIO()	{ return "Std-IO"; }
const char* StreamProvider::sStdErr()	{ return "Std-Err"; }
#define mCmdBufSz 8000

#ifndef __win__

#define mkUnLinked(fnm) fnm

#else

static const char* mkUnLinked( const char* fnm )
{
    if ( !fnm || !*fnm )
	return fnm;

    // Maybe the file itself is a link
    mDeclStaticString( ret );
    ret = File::linkTarget(fnm);
    if ( File::exists(ret) )
	return ret.buf();

    // Maybe there are links in the directories
    FilePath fp( fnm );
    int nrlvls = fp.nrLevels();
    for ( int idx=0; idx<nrlvls; idx++ )
    {
	BufferString dirnm = fp.dirUpTo( idx );
	const bool islink = File::isLink(dirnm);
	if ( islink )
	    dirnm = File::linkTarget( fp.dirUpTo(idx) );
	if ( !File::exists(dirnm) )
	    return fnm;

	if ( islink )
	{
	    FilePath fp2( dirnm );
	    for ( int ilvl=idx+1; ilvl<nrlvls; ilvl++ )
		fp2.add( fp.dir(ilvl) );

	    fp = fp2;
	    nrlvls = fp.nrLevels();
	}
    }

    ret = fp.fullPath();
    return ret.buf();
}

#endif


StreamProvider::StreamProvider( const char* fnm )
{
    setFileName( fnm );
}


StreamProvider::StreamProvider( const OS::MachineCommand& mc,
				const char* workdir )
{
    setCommand( mc, workdir );
}


StreamProvider::~StreamProvider()
{
    delete mc_;
}


static const char* extractHostName( const char* str, BufferString& hnm )
{
    hnm.setEmpty();
    if ( !str )
	return str;

    mSkipBlanks( str );
    BufferString inp( str );
    char* ptr = inp.getCStr();
    const char* rest = str;

#ifdef __win__

    if ( *ptr == '\\' && *(ptr+1) == '\\' )
    {
	ptr += 2;
	char* phnend = firstOcc( ptr, '\\' );
	if ( phnend ) *phnend = '\0';
	hnm = ptr;
	rest += hnm.size() + 2;
    }

#else

    while ( *ptr && !iswspace(*ptr) && *ptr != ':' )
	ptr++;

    if ( *ptr == ':' )
    {
	if ( *(ptr+1) == '/' && *(ptr+2) == '/' )
	{
	    inp.add( "\nlooks like a URL. Not supported (yet)" );
	    ErrMsg( inp ); rest += StringView(str).size();
	}
	else
	{
	    *ptr = '\0';
	    hnm = inp;
	    rest += hnm.size() + 1;
	}
    }

#endif

    return rest;
}


void StreamProvider::set( const char* inp )
{
    fname_.setEmpty();
    deleteAndZeroPtr( mc_ );

    BufferString workstr( inp );
    workstr.trimBlanks();
    if ( workstr.isEmpty() )
	return;

    if ( workstr == sStdIO() || workstr == sStdErr() )
    {
	fname_.set( workstr );
	return;
    }

    const char* pwork = workstr.buf();
    while ( *pwork == '@' )
    {
	pErrMsg("Deprecated. Use setCommand instead");
	DBG::forceCrash(false);
	return;
    }

    mSkipBlanks( pwork );
    fname_.set( pwork );
    if ( fname_.startsWith("file://",CaseInsensitive) )
	{ pwork += 7; fname_ = pwork; }

    BufferString hostname;
    workstr = extractHostName( fname_.buf(), hostname );

    pwork = workstr.buf();
    mSkipBlanks( pwork );
    while ( *pwork == '@' )
    {
	pErrMsg("Deprecated. Use setCommand instead");
	DBG::forceCrash(false);
	return;
    }

    fname_.set( pwork );
}


void StreamProvider::setFileName( const char* fnm )
{
    fname_.set( fnm );
    deleteAndZeroPtr( mc_ );
    workingdir_.setEmpty();
}


void StreamProvider::setCommand( const OS::MachineCommand& mc,
				 const char* workdir )
{
    fname_.setEmpty();
    if ( mc_ )
	*mc_ = mc;
    else
	mc_ = new OS::MachineCommand( mc );
    workingdir_.set( workdir );
}


bool StreamProvider::isBad() const
{
    return fname_.isEmpty() && !mc_;
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isBad() || mc_ || !path || ! *path
      || fname_ == sStdIO() || fname_ == sStdErr() )
	return;

    FilePath fp( fname_ );
    if ( !fp.isAbsolute() )
    {
	fp.insert( path );
	fname_ = fp.fullPath();
    }
}


StreamData StreamProvider::makeIStream( bool binary, bool dummyarg ) const
{
    return mc_ ? createCmdIStream( *mc_, workingdir_ )
	       : createIStream( fname_, binary );
}


StreamData StreamProvider::makeOStream( bool binary, bool editmode ) const
{
    return mc_ ? createCmdOStream( *mc_, workingdir_ )
	       : createOStream( fname_, binary, editmode );
}


StreamData StreamProvider::createIStream( const char* fnm, bool binary )
{
    const OD::FileSystemAccess& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.createIStream( fnm, binary );
}


StreamData StreamProvider::createOStream( const char* fnm,
				bool binary, bool editmode )
{
    const OD::FileSystemAccess& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.createOStream( fnm, binary, editmode );
}


StreamData StreamProvider::createCmdIStream( const OS::MachineCommand& mc,
					     const char* workdir,
					     bool fromstderr )
{
    StreamData retsd;

#ifndef OD_NO_QT
    const OS::MachineCommand usablemc = mc.getExecCommand();
    const QString qprog( usablemc.program() );
    QStringList qargs;
    usablemc.args().fill( qargs );

    QProcess* process = new QProcess;
    if ( !StringView(workdir).isEmpty() )
    {
	const QString qworkdir( workdir );
	process->setWorkingDirectory( qworkdir );
    }

    process->start( qprog, qargs, QIODevice::ReadOnly );
    if ( process->waitForStarted() )
    {
	qstreambuf* stdiosb = new qstreambuf( *process, fromstderr, true );
	retsd.setIStrm( new iqstream(stdiosb) );
    }
    else
	delete process;
#endif

    return retsd;

}


StreamData StreamProvider::createCmdOStream( const OS::MachineCommand& mc,
					     const char* workdir )
{
    StreamData retsd;

#ifndef OD_NO_QT
    const OS::MachineCommand usablemc = mc.getExecCommand();
    const QString qprog( usablemc.program() );
    QStringList qargs;
    usablemc.args().fill( qargs );

    QProcess* process = new QProcess;
    if ( !StringView(workdir).isEmpty() )
    {
	const QString qworkdir( workdir );
	process->setWorkingDirectory( qworkdir );
    }

    process->start( qprog, qargs, QIODevice::WriteOnly );
    if ( process->waitForStarted() )
    {
	qstreambuf* stdiosb = new qstreambuf( *process, false, true );
	retsd.setOStrm( new oqstream(stdiosb) );
    }
    else
	delete process;
#endif

    return retsd;

}


bool StreamProvider::exists( bool fr ) const
{
    if ( isBad() )
	return false;
    if ( mc_ )
	return fr;

    return fname_ == sStdIO() || fname_ == sStdErr() ? true
	 : File::exists( fname_ );
}


bool StreamProvider::remove( bool recursive ) const
{
    if ( isBad() || mc_ )
	return false;

    return fname_ == sStdIO() || fname_ == sStdErr() ? false
	 : File::remove( fname_ );
}


bool StreamProvider::setReadOnly( bool yn ) const
{
    if ( isBad() || mc_ )
	return false;

    return fname_ == sStdIO() || fname_ == sStdErr() ? false :
	   File::makeWritable( fname_, !yn, false );
}


bool StreamProvider::isReadOnly() const
{
    if ( isBad() || mc_ )
	return false;

    return fname_ == sStdIO() || fname_ == sStdErr() ? false :
	    !File::isWritable( fname_ );
}


static void mkRelocMsg( const char* oldnm, const char* newnm,BufferString& msg )
{
    msg = "Relocating '";
    while ( *oldnm && *newnm && *oldnm == *newnm )
	{ oldnm++; newnm++; }
    msg += oldnm; msg += "' to '"; msg += newnm; msg += "' ...";
}


void StreamProvider::sendCBMsg( const CallBack* cb, const char* msg )
{
    NamedCallBacker nobj( msg );
    CBCapsule<const char*> caps( msg, &nobj );
    CallBack(*cb).doCall( &caps );
}


bool StreamProvider::rename( const char* newnm, const CallBack* cb )
{
    const bool issane = newnm && *newnm && !isBad() && !mc_;

    if ( cb && cb->willCall() )
    {
	BufferString msg;
	if ( issane )
	    mkRelocMsg( fname_, newnm, msg );
	else if ( mc_ )
	    msg = "Cannot rename commands";
	else
	{
	    if ( isBad() )
		msg = "Cannot rename invalid filename";
	    else
		msg = "No filename provided for rename";
	}
	sendCBMsg( cb, msg );
    }
    if ( !issane )
	return false;

    bool isok = fname_ == sStdIO() || fname_ == sStdErr() ? true :
	    File::rename( fname_, newnm );
    if ( isok )
	setFileName( newnm );

    return isok;
}
