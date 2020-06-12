/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2007
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
#include "oscommand.h"
#include "progressmeterimpl.h"
#include "staticstring.h"
#include "uistrings.h"

const char* ODMad::ProcExec::sKeyFlowStage()	{ return "Flow Stage"; }
const char* ODMad::ProcExec::sKeyCurProc()	{ return "Current proc"; }

mDefineEnumUtils(ODMad::ProcExec,FlowStage,"Flow Stage")
{ "Start", "Intermediate", "Finish", 0 };

template<>
void EnumDefImpl<ODMad::ProcExec::FlowStage>::init()
{
    uistrings_ += uiStrings::sStart();
    uistrings_ += mEnumTr("Intermediate",0);
    uistrings_ += uiStrings::sFinish();
}

#ifdef __win__

/* Windows needs this approach as Madagascar binaries on Windows are not native
   Windows executables, they are Cygwin executables. */

#include <fstream>

std::ostream* makeOStream( const OS::MachineCommand& mc )
{
    BufferString cmd( mc.program() );
    BufferStringSet args( mc.args() );
    for ( auto arg : args )
    {
	if ( arg->find( " " ) && !arg->startsWith("\"") )
	    arg->quote( '\"' );
	cmd.addSpace().add( arg->str() );
    }

    FILE* fp = _popen(cmd, "w");
    bool ispipe = true;
    std::ostream* ostrm = 0;

    if ( fp )
    {
	std::filebuf* fb = new std::filebuf( fp );
	ostrm = new std::ostream(fb);
    }

    return ostrm;
}

#define mGetOStreamFromCmd(mc) new od_ostream( makeOStream(mc) )

#else // UNIX

#define mGetOStreamFromCmd(mc) new od_ostream( mc )

#endif


ODMad::ProcExec::ProcExec( const IOPar& iop, od_ostream& reportstrm )
    : Executor("Madagascar processing")
    , pars_(*new IOPar(iop))
    , strm_(reportstrm)
    , nrdone_(0)
    , stage_(Start)
    , madstream_(0)
    , procstream_(0)
    , plotstream_(0)
    , progmeter_(0)
    , trc_(0)
{
    FlowStageDef().parse( pars_.find( sKeyFlowStage() ), stage_ );
}


ODMad::ProcExec::~ProcExec()
{
    delete &pars_;
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
	    mErrRet( uiStrings::sParsMissing().addMoreInfo(uiStrings::sInput()))

	ODMad::ProcFlow::IOType inptyp = ODMad::ProcFlow::ioType( *inpar );
	OS::MachineCommand mc;
	if ( !getProcString(mc) )
	    return false;

	if ( stage_ == Start && ( inptyp == ODMad::ProcFlow::None
				|| inptyp == ODMad::ProcFlow::SU ) )
	{
	    OS::MachineCommand cmd;
	    if ( inptyp == ODMad::ProcFlow::SU )
	    {
		const BufferString rsfroot = GetEnvVar( "RSFROOT" );
		cmd.setProgram(
			File::Path( rsfroot, "bin", "sfsu2rsf" ).fullPath() );
		cmd.addArg( "tape=" ).addArg( inpar->find(sKey::FileName()) )
		   .addPipe().addArg( mc.program() ).addArgs( mc.args() );
	    }
	    else
		cmd = mc;

	    od_cerr() << "About to execute: " << cmd.toString() << od_endl;
	    return cmd.execute();
	}

	od_cerr() << "About to execute: " << mc.toString() << od_endl;

	if ( FixedString(mc.program()) == od_stream::sStdIO() )
	    procstream_ = new od_ostream( od_stream::sStdIO() );
	else
	    procstream_ = mGetOStreamFromCmd( mc );

	if ( !procstream_->isOK() )
	    mErrRet(tr("Failed to create output stream"))

	if ( !madstream_->putHeader(*procstream_) )
	    mErrRet(tr("Failed to get RSF header"))

	if ( stage_ == Intermediate )
	{
	    OS::MachineCommand plotmc;
	    getPlotString( plotmc );
	    od_cerr() << "About to plot: " << plotmc.toString() << od_endl;
	    plotstream_ = mGetOStreamFromCmd( plotmc );
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
        const BufferString fname = File::Path::getTempFullPath( "madproc", \
						sParFileExtension() ); \
	pars_.write( fname, sKey::Pars() ); \
	if ( mc.isBad() ) \
	{ \
	   mc.setProgram( \
		   File::Path(rsfroot).add("bin").add("sfdd").fullPath() ); \
	} \
	else \
	{ \
	   mc.addArg( \
		   File::Path(rsfroot).add("bin").add("sfdd").fullPath() ); \
	} \
	mc.addArg( "form=ascii_float" ).addPipe() \
	  .addArg( File::Path(GetExecPlfDir()).add("od_madexec").fullPath() ) \
	  .addArg( fname );
#else
    #define mAddNewExec \
	const BufferString fname = File::Path::getTempFullPath( "madproc", \
						sParFileExtension() ); \
	pars_.write( fname, sKey::Pars() ); \
	if ( mc.isBad() ) \
	    mc.setProgram( GetUnixExecScript() ); \
	else \
	    mc.addArg( GetUnixExecScript() ); \
	mc.addArg( "od_madexec" ).addArg( fname );
#endif

bool ODMad::ProcExec::getProcString( OS::MachineCommand& mc )
{
    const BufferString rsfroot = GetEnvVar( "RSFROOT" );
    ODMad::ProcFlow procflow;
    procflow.usePar( pars_ );
    ODMad::ProcFlow::IOType outtyp = procflow.ioType( false );
    int curprocidx = 0;
    if ( pars_.get(sKeyCurProc(),curprocidx) && curprocidx < 0 )
	return false;

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
	    mc.setProgram( procflow.output().find( sKey::FileName() ) );
	else if ( outtyp == ODMad::ProcFlow::None )
	    return false;
	else
	{
	    pars_.set( sKeyFlowStage(), toString(Finish) );
	    pars_.set( sKey::LogFile(), od_stream::sStdErr() );
	    mAddNewExec;
	}

	return mc.isBad();
    }

    bool firstproc = true;
    for ( int pidx=curprocidx; pidx<procflow.size(); pidx++ )
    {
	if ( !firstproc )
	    mc.addPipe();
	else
	{
#ifdef __win__
	    const File::Path fp( rsfroot, "bin", "sfdd" );
	    if ( mc.isBad() )
		mc.setProgram( fp.fullPath() );
	    else
		mc.addArg( fp.fullPath() );
	    mc.addArg( "form=native_float" ).addPipe();
#endif
	    firstproc = false;
	}

	if ( !procflow[pidx]->isValid() )
	{
	    if ( mc.isBad() )
		mc.setProgram( procflow[pidx]->getCommand() );
	    else
		mc.addArg( procflow[pidx]->getCommand() );
	}
	else
	{
	    const File::Path fp( rsfroot, "bin", procflow[pidx]->getCommand() );
	    if ( mc.isBad() )
		mc.setProgram( fp.fullPath() );
	    else
		mc.addArg( fp.fullPath() );
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
	    pars_.set( sKeyFlowStage(), toString(newstage) );
	    mc.addPipe();
	    if ( endproc && nooutput )
	    {
		getPlotString( mc );
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
		mc.addArg( "out=stdout" )
		  .addFileRedirect( procflow.output().find(sKey::FileName()) );
	    }
	    else
	    {
		pars_.set( sKeyFlowStage(), toString(Finish) );
		pars_.set( sKey::LogFile(), od_stream::sStdErr() );
		mc.addPipe();
		mAddNewExec;
	    }
	}
    }

    return !mc.isBad();
}


bool ODMad::ProcExec::getPlotString( OS::MachineCommand& mc ) const
{
    const BufferString rsfroot = GetEnvVar( "RSFROOT" );
    int curprocidx = 0;
    if ( !pars_.get(sKeyCurProc(),curprocidx) || curprocidx < 1 )
	return false;

    ODMad::ProcFlow procflow;
    procflow.usePar( pars_ );
    BufferString plotcmd = procflow[curprocidx-1]->auxCommand();
    if ( plotcmd.isEmpty() )
	return 0;

    File::Path fp( rsfroot, "bin" );
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
    mc.addArg( fp.fullPath() ).addArg( "&" );
    return !mc.isBad();
}


uiString ODMad::ProcExec::message() const
{ return uiStrings::sProcessing(); }


uiString ODMad::ProcExec::nrDoneText() const
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
