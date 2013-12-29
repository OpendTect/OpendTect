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
#include "staticstring.h"
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
    return OSCommand::defaultRemExec();
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
	    msg_ = "Failed to allocate "; msg_ += bufsz / 1024;
	    msg_ += " kb of memory";
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

const char* message() const { return msg_.buf(); }
const char* nrDoneText() const { return "MBs read"; }
od_int64 nrDone() const { return filesz_ / 1024; }
od_int64 totalNr() const { return dp_ ? dp_->size() / 1024 : -1; }

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

bool go( TaskRunner& tr )
{
    return TaskRunner::execute( &tr, *this );
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

const char* message() const	{ return curpld_ ? curpld_->message() : ""; }
const char* nrDoneText() const	{ return curpld_ ? curpld_->nrDoneText() : ""; }
od_int64 totalNr() const	{ return totnr_; }
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


bool StreamProvider::preLoad( const BufferStringSet& fnms, TaskRunner& tr,
				const char* id )
{
    if ( fnms.isEmpty() ) return true;

    StreamProviderDataPreLoader exec( fnms, id );
    return TaskRunner::execute( &tr, exec );
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


StreamProvider::StreamProvider( const char* hostnm, const char* fnm,
				bool iscomm )
    : iscomm_(iscomm)
    , hostname_(hostnm)
    , fname_(fnm?fnm:sStdIO())
{
}


void StreamProvider::set( const char* inp )
{
    hostname_.setEmpty(); fname_.setEmpty();
    iscomm_ = false;
    if ( !inp || !*inp )
    {
	if ( !inp )
	    fname_ = sStdIO();
	return;
    }

    BufferString workstr( inp );
    workstr.trimBlanks();
    if ( workstr.isEmpty() )
	return; // only spaces: invalid
    else if ( workstr == sStdIO() || workstr == sStdErr() )
	{ fname_ = workstr; return; }

    const char* pwork = workstr.buf();
    if ( *pwork == '@' )
	{ iscomm_ = true; pwork++; }

    mSkipBlanks( pwork );
    fname_ = pwork;

    workstr = OSCommand::extractHostName( fname_.buf(), hostname_ );
    pwork = workstr.buf();
    mSkipBlanks( pwork );
    if ( *pwork == '@' )
	{ iscomm_ = true; pwork++; }

    fname_ = pwork;
}


bool StreamProvider::isNormalFile() const
{
    return !iscomm_ && hostname_.isEmpty();
}


const char* StreamProvider::fullName() const
{
    mDeclStaticString( ret );
    ret.setEmpty();

    if ( iscomm_ )
	ret.add( "@" );
    if ( !hostname_.isEmpty() )
    {
#ifdef __win__
	ret.add( "\\\\" ).add( hostname_ );
#else
	ret.add( hostname_ ).add( ":" );
#endif
    }
    ret.add( fname_.buf() );

    return ret.buf();
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isBad() || iscomm_ || !path || ! *path
      || fname_ == sStdIO() || fname_ == sStdErr() )
	return;

    FilePath fp( fname_ );
    if ( fp.isAbsolute() )
	return;

    fp.insert( path );
    fname_ = fp.fullPath();
}


StreamData StreamProvider::makeIStream( bool binary, bool allowpl ) const
{
    const bool islocal = hostname_.isEmpty();
    StreamData sd;
    if ( !iscomm_ && islocal )
	sd.setFileName( mkUnLinked(fname_) );
    else
	sd.setFileName( fname_ );
    if ( isBad() )
	return sd;
    else if ( fname_ == sStdIO() || fname_ == sStdErr() )
	{ sd.istrm = &std::cin; return sd; }

    if ( allowpl )
    {
	const int plid = getPLID( sd.fileName(), false );
	if ( plid >= 0 )
	    return makePLIStream( plid );
    }

    if ( !iscomm_ && islocal )
    {
	bool doesexist = File::exists( sd.fileName() );
	if ( !doesexist )
	{
	    FilePath fp( fname_ );
	    BufferString fullpath = fp.fullPath( FilePath::Local, true );
	    if ( !File::exists(fullpath) )
		fullpath = fp.fullPath( FilePath::Local, false );
	    // Sometimes the filename _is_ weird, and the cleanup is wrong
	    doesexist = File::exists( fullpath );
	    if ( doesexist )
		sd.setFileName( fullpath );
	}

#ifdef __msvc__
	sd.istrm = new std::winifstream
#else
	sd.istrm = new std::ifstream
#endif
	  ( sd.fileName(), binary ? std::ios_base::in | std::ios_base::binary
				  : std::ios_base::in );

	if ( !sd.istrm->good() )
	    { delete sd.istrm; sd.istrm = 0; }
	return sd;
    }

    BufferString cmd;
    mkOSCmd( true, cmd );

    sd.fp_ = popen( cmd, "r" );
    sd.ispipe_ = true;

    if ( sd.fp_ )
    {
#ifdef __msvc__
	std::filebuf* fb = new std::filebuf( sd.fp_ );
	sd.istrm = new std::istream( fb );
#else
# if __GNUC__ > 2
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp_, std::ios_base::in );
	sd.istrm = new std::istream( stdiofb );
# else
	sd.istrm = new std::ifstream( fileno(sd.fp) );
# endif
#endif
    }

    return sd;
}


StreamData StreamProvider::makeOStream( bool binary, bool editmode ) const
{
    StreamData sd;
    sd.setFileName( mkUnLinked(fname_) );

    if ( isBad() )
	return sd;
    else if ( fname_ == sStdIO() )
	{ sd.ostrm = &std::cout; return sd; }
    else if ( fname_ == sStdErr() )
	{ sd.ostrm = &std::cerr; return sd; }

    if ( !iscomm_ && hostname_.isEmpty() )
    {
        std::ios_base::openmode openmode = std::ios_base::out;
        if ( binary )
            openmode = openmode | std::ios_base::binary;

        if ( editmode )
            openmode = openmode | std::ios_base::in;

#ifdef __msvc__
	sd.ostrm = new std::winofstream( sd.fileName(), openmode );
#else
	sd.ostrm = new std::ofstream( sd.fileName(), openmode );
#endif

	if ( !sd.ostrm->good() )
	    { delete sd.ostrm; sd.ostrm = 0; }
	return sd;
    }

    BufferString cmd;
    mkOSCmd( false, cmd );

    sd.fp_ = popen( cmd, "w" );
    sd.ispipe_ = true;

    if ( sd.fp_ )
    {
#ifdef __msvc__
	std::filebuf* fb = new std::filebuf( sd.fp_ );
	sd.ostrm = new std::ostream( fb );
#else
# if __GNUC__ > 2
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp_,std::ios_base::out );
	sd.ostrm = new std::ostream( stdiofb );
# else
	sd.ostrm = new std::ofstream( fileno(sd.fp) );
# endif
#endif
    }

    return sd;
}


void StreamProvider::mkOSCmd( bool forread, BufferString& cmd ) const
{
    if ( iscomm_ )
	cmd = OSCommand( fname_, hostname_ ).get();
    else
    {
	char buf[mCmdBufSz];
	if ( forread )
	    sprintf( buf, "%s %s cat %s", remExecCmd(),
			    hostname_.buf(), fname_.buf() );
	else
	    sprintf( buf, "%s %s tee %s > /dev/null", remExecCmd(),
			    hostname_.buf(), fname_.buf() );
	cmd = buf;
    }
}


#define mRemoteTest(act) \
    FILE* fp = popen( cmd, "r" ); \
    char c; fscanf( fp, "%c", &c ); \
    pclose( fp ); \
    act (c == '1')


bool StreamProvider::exists( bool fr ) const
{
    if ( isBad() )
	return false;
    if ( iscomm_ )
	return fr;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? true
	     : File::exists( fname_ );

    char cmd[mCmdBufSz];
    sprintf( cmd, "%s %s 'test -%c %s && echo 1'", remExecCmd(),
			hostname_.buf(), fr ? 'r' : 'w', fname_.buf() );
    mRemoteTest(return);
}


bool StreamProvider::remove( bool recursive ) const
{
    if ( isBad() || iscomm_ ) return false;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr()
	    ? false : File::remove( fname_ );

    char cmd[mCmdBufSz];
    sprintf( cmd, "%s %s '/bin/rm -%s %s && echo 1'", remExecCmd(),
		hostname_.buf(), recursive ? "r" : "", fname_.buf() );

    mRemoteTest(return);
}


bool StreamProvider::setReadOnly( bool yn ) const
{
    if ( isBad() || iscomm_ ) return false;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? false :
	       File::makeWritable( fname_, !yn, false );

    char cmd[mCmdBufSz];
    sprintf( cmd, "%s %s 'chmod %s %s && echo 1'", remExecCmd(),
		hostname_.buf(), yn ? "a-w" : "ug+w", fname_.buf() );

    mRemoteTest(return);
}


bool StreamProvider::isReadOnly() const
{
    if ( isBad() || iscomm_ ) return true;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? false :
		!File::isWritable( fname_ );

    char cmd[mCmdBufSz];
    sprintf( cmd, "%s %s 'test -w %s && echo 1'", remExecCmd(),
		hostname_.buf(), fname_.buf() );

    mRemoteTest(return !);
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
    bool rv = false;
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

    if ( issane )
    {
	if ( hostname_.isEmpty() )
	    rv = fname_ == sStdIO() || fname_ == sStdErr() ? true :
		    File::rename( fname_, newnm );
	else
	{
	    char cmd[mCmdBufSz];
	    sprintf( cmd, "%s %s '/bin/mv -f %s %s && echo 1'", remExecCmd(),
			hostname_.buf(), fname_.buf(), newnm );
	    mRemoteTest(rv =);
	}
    }

    if ( rv )
	set( newnm );
    return rv;
}
