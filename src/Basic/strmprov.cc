/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "strmprov.h"
#include "datapack.h"
#include "keystrs.h"
#include "iopar.h"
#include "envvars.h"
#include "oscommand.h"
#include "perthreadrepos.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#ifdef __win__
# include "winutils.h"
# include <windows.h>
# include <istream>


# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "winstreambuf.h"
# endif
#endif

#ifndef __msvc__
# include <ext/stdio_filebuf.h>
# define mStdIOFileBuf __gnu_cxx::stdio_filebuf<char>
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
#define mPreLoadChunkSz 8388608
		// 8 MB

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


static inline const char* remExecCmd()
{
    return OS::MachineCommand::defaultRemExec();
}


//---- Pre-loaded data ----


class StreamProviderPreLoadDataPack : public BufferDataPack
{
public:

StreamProviderPreLoadDataPack( char* b, od_int64 s,
				const char* nm, const char* ki )
    : BufferDataPack(b,s,"Pre-loaded file")
    , keyid_(ki)
{
    setName( nm );
}

void dumpInfo( IOPar& iop ) const
{
    BufferDataPack::dumpInfo( iop );
    iop.set( IOPar::compKey("Object",sKey::ID()), keyid_ );
}

    BufferString	keyid_;

};


#define mBytesToMB(bytes) ( bytes / (1024*1024) )
class StreamProviderPreLoadedData : public Executor
{
public:

StreamProviderPreLoadedData( const char* nm, const char* id )
    : Executor("Pre-loading data")
    , dp_(0)
    , id_(id)
    , filesz_(0)
    , chunkidx_(0)
    , msg_("Reading '")
    , fnm_(nm)
{
    FilePath fp( nm ); msg_ += fp.fileName(); msg_ += "'";

    sd_ = StreamProvider(nm).makeIStream(true,false);
    if ( !sd_.usable() )
	{ msg_ = "Cannot open '"; msg_ += nm; msg_ += "'"; }
    else
    {
	od_int64 bufsz = File::getKbSize( nm ) + 1;
	bufsz *= 1024;
	char* buf = 0;
	mTryAlloc(buf,char [ bufsz ])
	if ( !buf )
	{
	    msg_ = "Failed to allocate "; msg_ += mBytesToMB( bufsz );
	    msg_ += " MB of memory";
	}
	else
	{
	    dp_ = new StreamProviderPreLoadDataPack( buf, bufsz, nm, id );
	    DPM(DataPackMgr::BufID()).addAndObtain( dp_ );
	}
    }

    if ( !dp_ )
	sd_.close();
}

~StreamProviderPreLoadedData()
{
    sd_.close();
    if ( dp_ )
	DPM(DataPackMgr::BufID()).release( dp_->id() );
}

const BufferString& fileName() const
{
    return dp_ ? dp_->name() : fnm_;
}

uiString uiMessage() const { return msg_.buf(); }
uiString uiNrDoneText() const { return "MBs read"; }
od_int64 nrDone() const { return mBytesToMB( filesz_ ); }
od_int64 totalNr() const { return dp_ ? mBytesToMB(dp_->size()) : -1; }

int nextStep()
{
    if ( !isOK() ) return ErrorOccurred();

    std::istream& strm = *sd_.istrm;
    od_int64 offs = chunkidx_; offs *= mPreLoadChunkSz;
    strm.read( dp_->buf() + offs, mPreLoadChunkSz );
    chunkidx_++;
    if ( strm.good() )
    {
	filesz_ += mPreLoadChunkSz;
	return MoreToDo();
    }
    else if ( strm.eof() )
    {
	filesz_ += strm.gcount();
	sd_.close();
	return Finished();
    }

    msg_ = "Read error for '"; msg_ += fnm_; msg_ += "'";
    return ErrorOccurred();
}

bool go( TaskRunner& taskrunner )
{
    return TaskRunner::execute( &taskrunner, *this );
}

bool isOK() const
{
    return dp_;
}

    const BufferString	id_;
    StreamData		sd_;
    BufferDataPack*	dp_;
    int			chunkidx_;
    od_int64		filesz_;
    BufferString	msg_;
    BufferString	fnm_;

    char		buf_[mPreLoadChunkSz];

};


static ObjectSet<StreamProviderPreLoadedData>& PLDs()
{
    mDefineStaticLocalObject( ObjectSet<StreamProviderPreLoadedData>, plds, );
    return plds;
}


class StreamProviderDataPreLoader : public Executor
{
public:

StreamProviderDataPreLoader( const BufferStringSet& nms, const char* id )
    : Executor("Pre-loading data")
    , id_(id)
    , curpld_(0)
    , curnmidx_(-1)
    , totnr_(0)
    , nrdone_(0)
{
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	const char* fnm = nms.get( idx );
	if ( !File::exists(fnm) )
	    continue;

	fnms_.add( fnm );
	totnr_ += File::getKbSize( fnm );
    }
    mkNewPLD();
}

bool mkNewPLD()
{
    if ( curpld_ )
    {
	if ( curpld_->isOK() )
	{
	    PLDs() += curpld_;
	    nrdone_ += curpld_->nrDone();
	}
	else
	{
	    totnr_ -= File::getKbSize( curpld_->fnm_ );
	    delete curpld_;
	}
    }

    curpld_ = 0;
    curnmidx_++;

    if ( curnmidx_ < fnms_.size() )
    {
	const char* fnm = fnms_.get( curnmidx_ );
	if ( StreamProvider::isPreLoaded(fnm,false) )
	    StreamProvider::unLoad(fnm,false);
	curpld_ = new StreamProviderPreLoadedData( fnm, id_ );
    }

    return curpld_;
}

uiString uiMessage() const
{ return curpld_ ? curpld_->uiMessage() : uiString::emptyString(); }
uiString uiNrDoneText() const
{ return curpld_ ? curpld_->uiNrDoneText() : uiString::emptyString(); }
od_int64 totalNr() const	{ return totnr_ / 1024; }
od_int64 nrDone() const
{ return nrdone_ + (curpld_ ? curpld_->nrDone() : 0); }

int nextStep()
{
    if ( !curpld_ ) return Executor::Finished();

    int res = curpld_->nextStep();
    if ( res != Executor::Finished() )
	return res;
    else
	return mkNewPLD() ? Executor::MoreToDo() : Executor::Finished();
}

    const BufferString		id_;
    BufferStringSet		fnms_;
    int				curnmidx_;
    od_int64			totnr_;
    od_int64			nrdone_;
    StreamProviderPreLoadedData* curpld_;

};


static int getPLID( const char* key, bool isid )
{
    const ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( !pld.isOK() ) continue;

	if ( (!isid && pld.fileName() == key) || (isid && pld.id_ == key) )
	    return idx;
    }
    return -1;
}


bool StreamProvider::isPreLoaded( const char* key, bool isid )
{
    return getPLID(key,isid) >= 0;
}


bool StreamProvider::preLoad( const char* fnm, TaskRunner& tr, const char* id )
{
    if ( !fnm || !*fnm || isPreLoaded(fnm,false) ) return true;

    StreamProviderPreLoadedData* newpld =
			new StreamProviderPreLoadedData( fnm, id );
    if ( newpld->go(tr) && newpld->isOK() )
	PLDs() += newpld;
    else
	{ delete newpld; newpld = 0; }

    return newpld;
}


bool StreamProvider::preLoad( const BufferStringSet& fnms,
			      TaskRunner& taskrunner,
			      const char* id )
{
    if ( fnms.isEmpty() ) return true;

    StreamProviderDataPreLoader exec( fnms, id );
    return TaskRunner::execute( &taskrunner, exec );
}


void StreamProvider::unLoad( const char* key, bool isid )
{
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    while ( true )
    {
	int plid = getPLID( key, isid );
	if ( plid < 0 ) return;
	delete plds.removeSingle( plid, false );
    }
}


void StreamProvider::unLoadAll()
{
    deepErase( PLDs() );
}


void StreamProvider::getPreLoadedIDs( BufferStringSet& bss )
{
    bss.erase();
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( pld.isOK() )
	    bss.addIfNew( pld.id_ );
    }
}


void StreamProvider::getPreLoadedFileNames( const char* id,
					    BufferStringSet& bss )
{
    bss.erase();
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( !pld.isOK() ) continue;

	if ( !id || pld.id_ == id )
	    bss.add( pld.fileName() );
    }
}


int StreamProvider::getPreLoadedDataPackID( const char* fnm )
{
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( !pld.isOK() ) continue;

	if ( pld.fileName() == fnm )
	    return pld.dp_->id();
    }
    return -1;
}


StreamData StreamProvider::makePLIStream( int plid )
{
    StreamProviderPreLoadedData& pld = *PLDs()[plid];
    StreamData ret; ret.setFileName( pld.fileName() );
    std::fixedstreambuf* fsb
		= new std::fixedstreambuf( pld.dp_->buf(), pld.filesz_, false );
    ret.istrm = new std::istream( fsb );
    return ret;
}


StreamProvider::StreamProvider( const char* inp )
    : iscomm_(false)
{
    set( inp );
}


void StreamProvider::set( const char* inp )
{
    hostname_.setEmpty(); fname_.setEmpty();
    iscomm_ = false;
    if ( !inp || !*inp )
	return;

    BufferString workstr( inp );
    workstr.trimBlanks();
    if ( workstr.isEmpty() )
	return; // only spaces: invalid
    else if ( workstr == sStdIO() || workstr == sStdErr() )
	{ fname_ = workstr; return; }

    const char* pwork = workstr.buf();
    while ( *pwork == '@' )
	{ iscomm_ = true; pwork++; }

    mSkipBlanks( pwork );
    fname_ = pwork;

    workstr = OS::MachineCommand::extractHostName( fname_.buf(), hostname_ );

    pwork = workstr.buf();
    mSkipBlanks( pwork );
    while ( *pwork == '@' )
	{ iscomm_ = true; pwork++; }

    fname_ = pwork;
    if ( !iscomm_ )
	hostname_.setEmpty();
}


void StreamProvider::setFileName( const char* fnm )
{
    iscomm_ = false;
    fname_.set( fnm );
    hostname_.setEmpty();
}


void StreamProvider::setCommand( const char* cmd, const char* hostnm )
{
    iscomm_ = true;
    fname_.set( cmd );
    hostname_.set( hostnm );
}


const char* StreamProvider::fullName() const
{
    mDeclStaticString( ret );
    ret.setEmpty();

    if ( iscomm_ )
    {
	ret.add( "@" );
	if ( !hostname_.isEmpty() )
#ifdef __win__
	    ret.add( "\\\\" ).add( hostname_ )
#else
	    ret.add( hostname_ )
#endif
		.add( ":" );
    }
    ret.add( fname_ );

    return ret.buf();
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isBad() || iscomm_ || !path || ! *path
      || fname_ == sStdIO() || fname_ == sStdErr() )
	return;

    FilePath fp( fname_ );
    if ( !fp.isAbsolute() )
    {
	fp.insert( path );
	fname_ = fp.fullPath();
    }
}


#define mGetRetSD( retsd ) \
    StreamData retsd; \
    if ( iscomm_ ) \
	retsd.setFileName( BufferString("@",fname_) ); \
    else \
	retsd.setFileName( mkUnLinked(fname_.buf()) ); \
    if ( isBad() ) \
	return retsd


StreamData StreamProvider::makeIStream( bool binary, bool allowpl ) const
{
    mGetRetSD( retsd );

    if ( fname_ == sStdIO() || fname_ == sStdErr() )
	{ retsd.istrm = &std::cin; return retsd; }

    if ( !iscomm_ && allowpl )
    {
	const int plid = getPLID( retsd.fileName(), false );
	if ( plid >= 0 )
	    return makePLIStream( plid );
    }

    if ( !iscomm_ )
    {
	bool doesexist = File::exists( retsd.fileName() );
	if ( !doesexist )
	{
	    FilePath fp( fname_ );
	    BufferString fullpath = fp.fullPath( FilePath::Local, true );
	    if ( !File::exists(fullpath) )
		fullpath = fp.fullPath( FilePath::Local, false );
	    // Sometimes the filename _is_ weird, and the cleanup is wrong
	    doesexist = File::exists( fullpath );
	    if ( doesexist )
		retsd.setFileName( fullpath );
	}

#ifdef __msvc__
	retsd.istrm = new std::winifstream
#else
	retsd.istrm = new std::ifstream
#endif
	  ( retsd.fileName(), binary ? std::ios_base::in | std::ios_base::binary
				  : std::ios_base::in );

	if ( !retsd.istrm->good() )
	    { delete retsd.istrm; retsd.istrm = 0; }
	return retsd;
    }

    BufferString cmd;
    mkOSCmd( cmd );

    retsd.fileptr_ = popen( cmd, "r" );
    retsd.ispipe_ = true;

    if ( retsd.fileptr_ )
    {
#ifdef __msvc__
	std::filebuf* fb = new std::filebuf( (FILE*)retsd.fileptr_ );
	retsd.istrm = new std::istream( fb );
#else
	mStdIOFileBuf* stdiofb
		= new mStdIOFileBuf( (FILE*)retsd.fileptr_, std::ios::in );
	retsd.istrm = new std::istream( stdiofb );
#endif
    }

    return retsd;
}


StreamData StreamProvider::makeOStream( bool binary, bool editmode ) const
{
    mGetRetSD( retsd );

    if ( fname_ == sStdIO() )
	{ retsd.ostrm = &std::cout; return retsd; }
    else if ( fname_ == sStdErr() )
	{ retsd.ostrm = &std::cerr; return retsd; }

    if ( !iscomm_ )
    {
        std::ios_base::openmode openmode = std::ios_base::out;
        if ( binary )
            openmode = openmode | std::ios_base::binary;

        if ( editmode )
            openmode = openmode | std::ios_base::in;

#ifdef __msvc__
	if ( File::isHidden(retsd.fileName()) )
	    File::hide( retsd.fileName(), false );

	retsd.ostrm = new std::winofstream( retsd.fileName(), openmode );
#else
	retsd.ostrm = new std::ofstream( retsd.fileName(), openmode );
#endif

	if ( !retsd.ostrm->good() )
	    { delete retsd.ostrm; retsd.ostrm = 0; }
	return retsd;
    }

    BufferString cmd;
    mkOSCmd( cmd );

    retsd.fileptr_ = popen( cmd, "w" );
    retsd.ispipe_ = true;

    if ( retsd.fileptr_ )
    {
#ifdef __msvc__
	std::filebuf* fb = new std::filebuf( (FILE*)retsd.fileptr_ );
	retsd.ostrm = new std::ostream( fb );
#else
	mStdIOFileBuf* stdiofb
		    = new mStdIOFileBuf( (FILE*)retsd.fileptr_,std::ios::out);
	retsd.ostrm = new std::ostream( stdiofb );
#endif
    }

    return retsd;
}


void StreamProvider::mkOSCmd( BufferString& cmd ) const
{
    if ( hostname_.isEmpty() )
	cmd = fname_;
    else
	cmd.set( remExecCmd() ).add( " " ).add( hostname_ )
				.add( " " ).add( fname_ );
}


bool StreamProvider::exists( bool fr ) const
{
    if ( isBad() )
	return false;
    if ( iscomm_ )
	return fr;

    return fname_ == sStdIO() || fname_ == sStdErr() ? true
	 : File::exists( fname_ );
}


bool StreamProvider::remove( bool recursive ) const
{
    if ( isBad() || iscomm_ )
	return false;

    return fname_ == sStdIO() || fname_ == sStdErr() ? false
	 : File::remove( fname_ );
}


bool StreamProvider::setReadOnly( bool yn ) const
{
    if ( isBad() || iscomm_ )
	return false;

    return fname_ == sStdIO() || fname_ == sStdErr() ? false :
	   File::makeWritable( fname_, !yn, false );
}


bool StreamProvider::isReadOnly() const
{
    if ( isBad() || iscomm_ )
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
    NamedObject nobj( msg );
    CBCapsule<const char*> caps( msg, &nobj );
    CallBack(*cb).doCall( &caps );
}


bool StreamProvider::rename( const char* newnm, const CallBack* cb )
{
    const bool issane = newnm && *newnm && !isBad() && !iscomm_;

    if ( cb && cb->willCall() )
    {
	BufferString msg;
	if ( issane )
	    mkRelocMsg( fname_, newnm, msg );
	else if ( iscomm_ )
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
	set( newnm );

    return isok;
}
