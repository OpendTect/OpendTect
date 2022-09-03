/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "envvars.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "madprocexec.h"
#include "madprocflow.h"
#include "madstream.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "progressmeter.h"
#include "perthreadrepos.h"
#include "uistrings.h"

const char* ODMad::ProcExec::sKeyFlowStage()	{ return "Flow Stage"; }
const char* ODMad::ProcExec::sKeyCurProc()	{ return "Current proc"; }

mDefineEnumUtils(ODMad::ProcExec,FlowStage,"Flow Stage")
{ "Start", "Intermediate", "Finish", 0 };

/* Windows needs this approach as Madagascar binaries on Windows are not native
   Windows executables, they are Cygwin executables. */

#ifdef __win__

# define mPOpen _popen
#define mPClose _pclose
# include "../../src/Basic/winstreambuf.h"
# define mStdIOFileBuf std::filebuf
#define mFPArgs fp

#elif __lux__

# include <fstream>

# define mPOpen popen
# define mPClose pclose
# include <ext/stdio_filebuf.h>
# define mStdIOFileBuf __gnu_cxx::stdio_filebuf<char>
# define mFPArgs  fp,std::ios::out

#endif

static od_ostream* getOStreamFromCmd( const char* comm, FILE*& fp )
{
#ifdef __mac__
    return new od_ostream( comm );
#else
    std::ostream* ostrm = 0;
    BufferString cmd( comm );

    fp = mPOpen( cmd, "w" );

    if ( fp )
    {
	mStdIOFileBuf* fb = new mStdIOFileBuf( mFPArgs );
	ostrm = new std::ostream( fb );
    }

    return new od_ostream( ostrm );
#endif
}


ODMad::ProcExec::ProcExec( const IOPar& iop, od_ostream& reportstrm )
    : Executor("Madagascar processing")
    , pars_(*new IOPar(iop))
    , strm_(reportstrm)
    , nrdone_(0)
    , stage_(Start)
    , madstream_(0)
    , procfptr_(0)
    , plotfptr_(0)
    , procstream_(0)
    , plotstream_(0)
    , progmeter_(0)
    , trc_(0)
{
    parseEnumFlowStage( pars_.find( sKeyFlowStage() ), stage_ );
}


ODMad::ProcExec::~ProcExec()
{
    delete &pars_;
    delete procfptr_; delete plotfptr_;
    delete procstream_; delete plotstream_;
    delete madstream_; delete progmeter_;
    delete [] trc_;
}


#define mErrRet(s) { errmsg_ = s; return false; }
bool ODMad::ProcExec::init()
{
    delete madstream_; madstream_ = 0;
    if ( stage_ == Finish )
    {
	pars_.setYN( "Write", true );
	madstream_ = new ODMad::MadStream( pars_ );
	if ( !madstream_->isOK() )
	    mErrRet( madstream_->errMsg() )

	if ( !madstream_->writeTraces() )
	    mErrRet( madstream_->errMsg() )
    }
    else
    {
	madstream_ = new ODMad::MadStream( pars_ );
	if ( !madstream_->isOK() )
	    mErrRet( madstream_->errMsg() )

	PtrMan<IOPar> inpar = pars_.subselect( ODMad::ProcFlow::sKeyInp() );
	if ( !inpar || !inpar->size() )
	    mErrRet(uiStrings::sInputParamsMissing())

	ODMad::ProcFlow::IOType inptyp = ODMad::ProcFlow::ioType( *inpar );
	const char* comm = getProcString();
	if ( !comm || !*comm )
	    return false;

	od_cerr() << "About to execute: " << comm << od_endl;
	if ( stage_ == Start && ( inptyp == ODMad::ProcFlow::None
				|| inptyp == ODMad::ProcFlow::SU ) )
	{
	    BufferString cmd( comm + 1 );
	    if ( inptyp == ODMad::ProcFlow::SU )
	    {
		const BufferString rsfroot = GetEnvVar( "RSFROOT" );
		cmd = FilePath( rsfroot, "bin", "sfsu2rsf" ).fullPath();
		cmd += " "; cmd += "tape=";
		cmd += inpar->find( sKey::FileName() );
		cmd += " | "; cmd += comm + 1;
	    }

	    return !system( cmd.buf() );
	}

	if ( StringView(comm) == od_stream::sStdIO() )
	    procstream_ = new od_ostream( od_stream::sStdIO() );
	else
	    procstream_ = getOStreamFromCmd( comm, procfptr_ );

	if ( !procstream_->isOK() )
	    mErrRet(tr("Failed to create output stream"))

	if ( !madstream_->putHeader(*procstream_) )
	    mErrRet(tr("Failed to get RSF header"))

	if ( stage_ == Intermediate )
	{
	    BufferString plotcomm = getPlotString();
	    od_cerr() << "About to plot: " << plotcomm << od_endl;
	    plotstream_ = getOStreamFromCmd( plotcomm.buf(), plotfptr_ );
	    if ( !plotstream_->isOK() )
		mErrRet(tr("Failed to create plot output stream"))
	    if ( !madstream_->putHeader(*plotstream_) )
		mErrRet(tr("Failed to put RSF header in plot stream"))
	}

	const int trcsize = madstream_->getNrSamples();
	trc_ = new float[trcsize];
    }

    return true;
}


#ifdef __win__
    #define mAddNewExec \
        BufferString fname = FilePath::getTempFullPath( "madexec", "par" ); \
	pars_.write( fname, sKey::Pars() ); \
	ret += FilePath(rsfroot).add("bin").add("sfdd").fullPath(); \
	ret += " form=ascii_float | \""; \
	ret += FilePath(GetExecPlfDir()).add("od_madexec").fullPath(); \
	ret += "\" "; ret += fname
#else
    #define mAddNewExec \
	BufferString fname = FilePath::getTempFullPath( "madproc", \
						sParFileExtension() ); \
	pars_.write( fname, sKey::Pars() ); \
	ret += GetExecScript( false ); ret += " "; \
	ret += "od_madexec"; ret += " "; ret += fname
#endif

const char* ODMad::ProcExec::getProcString()
{
    const BufferString rsfroot = GetEnvVar( "RSFROOT" );
    mDeclStaticString( ret );
    ret.setEmpty();
    ODMad::ProcFlow procflow;
    procflow.usePar( pars_ );
    ODMad::ProcFlow::IOType outtyp = procflow.ioType( false );
    int curprocidx = 0;
    if ( pars_.get(sKeyCurProc(),curprocidx) && curprocidx < 0 )
	return 0;

    if ( !curprocidx )
    {
	pars_.write( strm_, IOPar::sKeyDumpPretty() );
	progmeter_ = new TextStreamProgressMeter( strm_ );
	progmeter_->setName( "Madagascar Processing:" );
	progmeter_->setMessage( m3Dots(tr("Reading Traces")) );
    }

    const bool hasprocessing = procflow.size() && curprocidx < procflow.size();
    if ( !hasprocessing )
    {
	if ( outtyp == ODMad::ProcFlow::Madagascar )
	    ret = procflow.output().find( sKey::FileName() );
	else if ( outtyp == ODMad::ProcFlow::None )
	    return nullptr;
	else
	{
	    pars_.set( sKeyFlowStage(), getFlowStageString(Finish) );
	    pars_.set( sKey::LogFile(), od_stream::sStdErr() );
	    mAddNewExec;
	}

	return ret.buf();
    }

    bool firstproc = true;
    for ( int pidx=curprocidx; pidx<procflow.size(); pidx++ )
    {
	if ( !firstproc )
	    ret += " | ";
	else
	{
#ifdef __win__
	    ret += FilePath(rsfroot).add("bin").add("sfdd").fullPath();
	    ret += " form=native_float | ";
#endif
	    firstproc = false;
	}

	if ( !procflow[pidx]->isValid() )
	    ret += procflow[pidx]->getCommand();
	else
	{
	    const FilePath fp( rsfroot, "bin", procflow[pidx]->getCommand() );
	    ret += fp.fullPath();
	}

	const char* plotcmd = procflow[pidx]->auxCommand();
	const bool hasplot = plotcmd && *plotcmd;
	const bool endproc = pidx == procflow.size()-1;
	const bool nooutput = outtyp == ODMad::ProcFlow::None;
	if ( hasplot )
	{
	    FlowStage newstage = endproc && nooutput ? Finish : Intermediate;
	    pars_.set( sKeyCurProc(), pidx + 1 );
	    pars_.set( sKey::LogFile(), od_stream::sStdErr() );
	    pars_.set( IOPar::compKey(ODMad::ProcFlow::sKeyInp(),sKey::Type() ),
		       "Madagascar" );
	    pars_.removeWithKey( IOPar::compKey(ODMad::ProcFlow::sKeyInp(),
						 sKey::FileName()) );
	    pars_.set( sKeyFlowStage(), getFlowStageString(newstage) );
	    ret += " | ";
	    if ( endproc && nooutput )
	    {
		ret += getPlotString();
		break;
	    }

	    mAddNewExec;
	    pars_.set( sKeyCurProc(), curprocidx );
	    break;
	}

	if ( endproc )
	{
	    if ( outtyp == ODMad::ProcFlow::Madagascar )
	    {
		ret += " out=stdout > ";
		ret += procflow.output().find( sKey::FileName() );
	    }
	    else
	    {
		pars_.set( sKeyFlowStage(), getFlowStageString(Finish) );
		pars_.set( sKey::LogFile(), od_stream::sStdErr() );
		ret += " | ";
		mAddNewExec;
	    }
	}
    }

    return ret.buf();
}


const char* ODMad::ProcExec::getPlotString() const
{
    const BufferString rsfroot = GetEnvVar( "RSFROOT" );
    mDeclStaticString( ret );
    int curprocidx = 0;
    if ( !pars_.get(sKeyCurProc(),curprocidx) || curprocidx < 1 )
	return 0;

    ODMad::ProcFlow procflow;
    procflow.usePar( pars_ );
    BufferString plotcmd = procflow[curprocidx-1]->auxCommand();
    if ( plotcmd.isEmpty() )
	return 0;

    FilePath fp( rsfroot, "bin" );
    char* pipechar = plotcmd.find( '|' );
    if ( pipechar )
    {
	BufferString tmp = pipechar + 2;
	*(pipechar+2) = '\0';
	fp.add( tmp );
	plotcmd += fp.fullPath();
	fp.setFileName( 0 );
    }

    fp.add( plotcmd );
    ret += fp.fullPath();
    ret += "&";
    return ret.buf();
}


uiString ODMad::ProcExec::uiMessage() const
{ return tr("Working"); }


uiString ODMad::ProcExec::uiNrDoneText() const
{ return tr("Traces handled"); }


od_int64 ODMad::ProcExec::totalNr() const
{
    return -1;
}

#define mWriteToStream(strm) \
    if ( madstream_->isBinary() ) \
        strm->addBin( (const char*) &val, sizeof(val)); \
    else \
	*strm << val << " ";

int ODMad::ProcExec::nextStep()
{
    if ( stage_ == Finish || !madstream_ || !madstream_->getNextTrace(trc_) )
    {
	deleteAndZeroPtr( procstream_ );
	deleteAndZeroPtr( plotstream_ );
	if ( progmeter_ ) progmeter_->setFinished();
	return Executor::Finished();
    }

    const int trcsize = madstream_->getNrSamples();
    for ( int idx=0; idx<trcsize; idx++ )
    {
	const float val = trc_[idx];
	mWriteToStream( procstream_ )
	if ( stage_ == Intermediate && plotstream_->isOK() )
	{ mWriteToStream( plotstream_ ) }
    }

    if ( progmeter_ ) progmeter_->setNrDone( ++nrdone_ );
    return Executor::MoreToDo();
}
