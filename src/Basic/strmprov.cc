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
#include "oscommand.h"
#include "perthreadrepos.h"
#include "strmdata.h"
#include "uistrings.h"
#include <iostream>


#ifdef __win__
# include <windows.h>
# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "winstreambuf.h"
# endif
#endif

#include <fstream>


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
{ mODTextTranslationClass(StreamProviderPreLoadedData);
public:

StreamProviderPreLoadedData( const char* nm, const char* id )
    : Executor("Pre-loading data")
    , dp_(0)
    , id_(id)
    , filesz_(0)
    , chunkidx_(0)
    , msg_(tr("Reading '"))
    , fnm_(nm)
{
    FilePath fp(nm); msg_.append(fp.fileName()).arg(uiString::emptyString());

    sd_ = StreamProvider(nm).makeIStream(true,false);
    if ( !sd_.usable() )
	{ msg_ = tr("Cannot open '%1'").arg(nm); }
    else
    {
	od_int64 bufsz = File::getKbSize( nm ) + 1;
	bufsz *= 1024;
	char* buf = 0;
	mTryAlloc(buf,char [ bufsz ])
	if ( !buf )
	{
	    msg_ = tr("Failed to allocate %1 MB of memory")
		 .arg(mBytesToMB( bufsz ));
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

const OD::String& fileName() const
{
    return dp_ ? dp_->name() : fnm_;
}

uiString uiMessage() const { return msg_; }
uiString uiNrDoneText() const { return tr("MBs read"); }
od_int64 nrDone() const { return mBytesToMB( filesz_ ); }
od_int64 totalNr() const { return dp_ ? mBytesToMB(dp_->size()) : -1; }

int nextStep()
{
    if ( !isOK() ) return ErrorOccurred();

    std::istream& strm = *sd_.iStrm();
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

    msg_ = tr( "Read error for '%1'").arg( fnm_ );
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
    uiString		msg_;
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

~StreamProviderDataPreLoader()
{
    delete curpld_;
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
    auto* fsb = new std::fixedstreambuf( pld.dp_->buf(), pld.filesz_, false );
    ret.setIStrm( new std::fixedistream( fsb ) );
    return ret;
}


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
	    ErrMsg( inp ); rest += FixedString(str).size();
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
	{ fname_.set( workstr ); return; }

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


#define mGetRetSD( retsd ) \
    StreamData retsd; \
    if ( mc_ ) \
    { \
	BufferString fnm( "@", mc_->program() ); \
	if ( !mc_->args().isEmpty() ) \
	    fnm.addSpace().add( mc_->args().cat(" ") ); \
	retsd.setFileName( fnm.str() ); \
    } \
    else \
	retsd.setFileName( mkUnLinked(fname_.buf()) ); \
    if ( isBad() ) \
	return retsd


StreamData StreamProvider::makeIStream( bool binary, bool allowpl ) const
{
    mGetRetSD( retsd );

    if ( fname_ == sStdIO() || fname_ == sStdErr() )
	{ retsd.setIStrm( &std::cin ); return retsd; }

    if ( !mc_ && allowpl )
    {
	const int plid = getPLID( retsd.fileName(), false );
	if ( plid >= 0 )
	    return makePLIStream( plid );
    }

    if ( !mc_ )
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
	retsd.setIStrm( new std::winifstream
#else
	retsd.setIStrm( new std::ifstream
#endif
	  ( retsd.fileName(), binary ? std::ios_base::in | std::ios_base::binary
				  : std::ios_base::in ) );

	if ( !retsd.iStrm()->good() )
	    { delete retsd.iStrm(); retsd.setIStrm( 0 ); }
	return retsd;
    }

#ifndef OD_NO_QT
    const OS::MachineCommand mc = mc_->getExecCommand();
    const QString qprog( mc.program() );
    QStringList qargs;
    mc.args().fill( qargs );

    QProcess* process = new QProcess;
    if ( !workingdir_.isEmpty() )
    {
	const QString qworkdir( workingdir_ );
	process->setWorkingDirectory( qworkdir );
    }

    process->start( qprog, qargs, QIODevice::ReadOnly );
    if ( process->waitForStarted() )
    {
	qstreambuf* stdiosb = new qstreambuf( *process, false, true );
	retsd.setIStrm( new iqstream( stdiosb ) );
    }
    else
	delete process;
#endif

    return retsd;
}


StreamData StreamProvider::makeOStream( bool binary, bool editmode ) const
{
    mGetRetSD( retsd );

    if ( fname_ == sStdIO() )
	{ retsd.setOStrm( &std::cout ); return retsd; }
    else if ( fname_ == sStdErr() )
	{ retsd.setOStrm( &std::cerr ); return retsd; }

    if ( !mc_ )
    {
	std::ios_base::openmode openmode = std::ios_base::out;
	if ( binary )
	    openmode = openmode | std::ios_base::binary;

	if ( editmode )
	    openmode = openmode | std::ios_base::in;

#ifdef __msvc__
	if ( File::isHidden(retsd.fileName()) )
	    File::hide( retsd.fileName(), false );

	retsd.setOStrm( new std::winofstream( retsd.fileName(), openmode ) );
#else
	retsd.setOStrm( new std::ofstream( retsd.fileName(), openmode ) );
#endif

	if ( !retsd.oStrm()->good() )
	    { delete retsd.oStrm(); retsd.setOStrm( 0 ); }
	return retsd;
    }

#ifndef OD_NO_QT
    const OS::MachineCommand mc = mc_->getExecCommand();
    const QString qprog( mc.program() );
    QStringList qargs;
    mc.args().fill( qargs );

    QProcess* process = new QProcess;
    if ( !workingdir_.isEmpty() )
    {
	const QString qworkdir( workingdir_ );
	process->setWorkingDirectory( qworkdir );
    }

    process->start( qprog, qargs, QIODevice::WriteOnly );
    if ( process->waitForStarted() )
    {
	qstreambuf* stdiosb = new qstreambuf( *process, false, true );
	retsd.setOStrm( new oqstream( stdiosb ) );
    }
    else
	delete process;
#endif

    return retsd;
}


StreamData StreamProvider::createIStream( const char* fnm, bool binary )
{
    if ( !fnm || !*fnm )
	return StreamData();

    StreamData res;
    auto* impl = new StreamData::StreamDataImpl;
    impl->fname_ = fnm;

    if ( !File::exists(fnm) )
    {
	const FilePath fp( fnm );
	BufferString fullpath = fp.fullPath( FilePath::Local, true );
	if ( !File::exists(fullpath) )
	    fullpath = fp.fullPath( FilePath::Local, false );
	// Sometimes the filename _is_ weired, and the cleanup is wrong
	if ( File::exists(fullpath) )
	    impl->fname_ = fullpath;
    }

    std::ios_base::openmode openmode = std::ios_base::in;
    if ( binary )
	openmode = openmode | std::ios_base::binary;

    deleteAndZeroPtr( impl->istrm_ );
#ifdef __msvc__
    impl->istrm_ = new std::winifstream( impl->fname_, openmode );
#else
    impl->istrm_ = new std::ifstream( impl->fname_, openmode );
#endif

    if ( !impl->istrm_ || !impl->istrm_->good() )
	deleteAndZeroPtr( impl->istrm_ );

    res.setImpl( impl );
    return res;

}


StreamData StreamProvider::createOStream( const char* fnm,
				bool binary, bool editmode )
{
    if ( !fnm || !*fnm )
	return StreamData();

    StreamData res;
    auto* impl = new StreamData::StreamDataImpl;
    impl->fname_ = fnm;
    std::ios_base::openmode openmode = std::ios_base::out;
    if ( binary )
	openmode |= std::ios_base::binary;

    if ( editmode )
	openmode |= std::ios_base::in;

#ifdef __msvc__
    if ( File::isHidden(fnm) )
	File::hide( fnm, false );

    impl->ostrm_ = new std::winofstream( fnm, openmode );
#else
    impl->ostrm_ = new std::ofstream( fnm, openmode );
#endif

    if ( !impl->ostrm_ || !impl->ostrm_->good() )
	deleteAndZeroPtr( impl->ostrm_ );

    res.setImpl( impl );

    return res;
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
