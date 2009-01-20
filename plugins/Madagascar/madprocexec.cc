/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Dec 2007
-*/

static const char* rcsID = "$Id: madprocexec.cc,v 1.4 2009-01-20 10:54:43 cvsraman Exp $";

#include "envvars.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "madprocexec.h"
#include "madprocflow.h"
#include "madstream.h"
#include "oddirs.h"
#include "strmprov.h"
#include "progressmeter.h"
#include <iostream>

static const char* sKeyInp = "Input";
const char* ODMad::ProcExec::sKeyFlowStage()	{ return "Flow Stage"; }
const char* ODMad::ProcExec::sKeyCurProc()	{ return "Current proc"; }

DefineEnumNames(ODMad::ProcExec,FlowStage,1,"Flow Stage")
{ "Start", "Intermediate", "Finish", 0 };


ODMad::ProcExec::ProcExec( const IOPar& pars, std::ostream& reportstrm )
    : Executor("Madagascar processing")
    , pars_(*new IOPar(pars))
    , strm_(reportstrm)
    , nrdone_(0)
    , stage_(Start)
    , madstream_(0)
    , procstream_(*new StreamData)
    , plotstream_(*new StreamData)
    , progmeter_(0)
{
    const char* str = pars_.find( sKeyFlowStage() );
    if ( str && *str )
	stage_ = eEnum(ODMad::ProcExec::FlowStage,str);
}


ODMad::ProcExec::~ProcExec()
{
    delete &pars_;
    delete &procstream_; delete &plotstream_;
    delete madstream_; delete progmeter_;
    delete [] trc_;
}


bool ODMad::ProcExec::init()
{
    delete madstream_; madstream_ = 0;
    if ( stage_ == Finish )
    {
	pars_.setYN( "Write", true );
	madstream_ = new ODMad::MadStream( pars_ );
	if ( !madstream_->isOK() )
	    return false;

	if ( !madstream_->writeTraces() )
	    return false;
    }
    else
    {
	madstream_ = new ODMad::MadStream( pars_ );
	if ( !madstream_->isOK() )
	    return false;

	PtrMan<IOPar> inpar = pars_.subselect( ODMad::ProcFlow::sKeyInp() );
	if ( !inpar || !inpar->size() )
	    return false;

	ODMad::ProcFlow::IOType inptyp = ODMad::ProcFlow::ioType( *inpar );
	const char* comm = getProcString();
	if ( !comm || !*comm )
	    return false;

	std::cerr << "Command = " << comm << std::endl;
	if ( stage_ == Start && ( inptyp == ODMad::ProcFlow::None
		    		|| inptyp == ODMad::ProcFlow::SU ) )
	{
	    BufferString cmd( comm + 1 );
	    if ( inptyp == ODMad::ProcFlow::SU )
	    {
		const char* rsfroot = GetEnvVar( "RSFROOT" );
		FilePath fp( rsfroot ); fp.add( "bin" );
		fp.add( "sfsu2rsf" );
		cmd = fp.fullPath();
		cmd += " "; cmd += "tape=";
		cmd += inpar->find( sKey::FileName );
		cmd += " | "; cmd += comm + 1;
	    }

	    return !system( cmd.buf() );
	}

	if ( !strcmp(comm,StreamProvider::sStdIO()) )
	    procstream_.ostrm = &std::cout;
	else
	    procstream_ = StreamProvider( comm ).makeOStream();

	if ( !madstream_->putHeader(*procstream_.ostrm) )
	{
	    strm_ << "Failed to get RSF header" << std::endl;
	    return false;
	}

	if ( stage_ == Intermediate )
	{
	    BufferString plotcomm = "@";
	    plotcomm += getPlotString();
	    plotstream_ = StreamProvider( plotcomm.buf() ).makeOStream();
	    if ( !madstream_->putHeader(*plotstream_.ostrm) )
		return false;
	}

	const int trcsize = madstream_->getNrSamples();
	trc_ = new float[trcsize];
    }

    return true;
}


#define mAddNewExec \
    BufferString fname = FilePath::getTempName( "par" ); \
    pars_.write( fname, sKey::Pars ); \
    ret += GetExecScript( false ); \
    ret += " "; ret += "odmadexec"; \
    ret += " "; ret += fname

const char* ODMad::ProcExec::getProcString()
{
    const char* rsfroot = GetEnvVar( "RSFROOT" );
    static BufferString ret( "@" );
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
	progmeter_->setMessage( "Reading Traces..." );
    }

    const bool hasprocessing = procflow.size() && curprocidx < procflow.size();
    if ( !hasprocessing )
    {
	if ( outtyp == ODMad::ProcFlow::Madagascar )
	    ret = procflow.output().find( sKey::FileName );
	else if ( outtyp == ODMad::ProcFlow::None )
	    return 0;
	else
	{
	    pars_.set( sKeyFlowStage(), eString(ODMad::ProcExec::FlowStage,
						Finish) );
	    pars_.set( sKey::LogFile, StreamProvider::sStdErr() );
	    ret = "";
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
	    firstproc = false;


	if ( !procflow[pidx]->isValid() )
	    ret += procflow[pidx]->getCommand();
	else
	{
	    FilePath fp( rsfroot ); fp.add( "bin" );
	    fp.add( procflow[pidx]->getCommand() );
	    ret += fp.fullPath();
	}

	const char* plotcmd = procflow[pidx]->auxCommand();
	const bool hasplot = plotcmd && *plotcmd;
	const bool endproc = pidx == procflow.size()-1;
	const bool nooutput = procflow.ioType(false) == ODMad::ProcFlow::None;
	if ( hasplot )
	{
	    FlowStage newstage = endproc && nooutput ? Finish : Intermediate;
	    pars_.set( sKeyCurProc(), pidx + 1 );
	    pars_.set( sKey::LogFile, StreamProvider::sStdErr() );
	    pars_.set( sKeyFlowStage(), eString(ODMad::ProcExec::FlowStage,
						newstage) );
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
		ret += " > ";
		ret += procflow.output().find( sKey::FileName );
	    }
	    else
	    {	    
		pars_.set( sKeyFlowStage(), eString(ODMad::ProcExec::FlowStage,
						    Finish) );
		pars_.set( sKey::LogFile, StreamProvider::sStdErr() );
		ret += " | ";
		mAddNewExec;
	    }
	}
    }

    return ret.buf();
}


const char* ODMad::ProcExec::getPlotString() const
{
    const char* rsfroot = GetEnvVar( "RSFROOT" );
    static BufferString ret;
    int curprocidx = 0;
    if ( !pars_.get(sKeyCurProc(),curprocidx) || curprocidx < 1 )
	return 0;

    ODMad::ProcFlow procflow;
    procflow.usePar( pars_ );
    BufferString plotcmd = procflow[curprocidx-1]->auxCommand();
    if ( plotcmd.isEmpty() )
	return 0;

    FilePath fp( rsfroot ); fp.add( "bin" );
    char* pipechar = strchr( plotcmd.buf(), '|' );
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
    return ret.buf();
}


const char* ODMad::ProcExec::message() const
{
    return "Working";
}


const char* ODMad::ProcExec::nrDoneText() const
{
    return "Traces handled";
}


od_int64 ODMad::ProcExec::totalNr() const
{
    return -1;
}


int ODMad::ProcExec::nextStep()
{
    if ( stage_ == Finish || !madstream_ || !madstream_->getNextTrace(trc_) )
    {
	procstream_.close();
	plotstream_.close();
	if ( progmeter_ ) progmeter_->setFinished();
	return Executor::Finished();
    }

    const int trcsize = madstream_->getNrSamples();
    for ( int idx=0; idx<trcsize; idx++ )
    {
	const float val = trc_[idx];
	(*procstream_.ostrm).write( (const char*) &val, sizeof(val));
	if ( stage_ == Intermediate && plotstream_.usable() )
	    (*plotstream_.ostrm).write( (const char*) &val, sizeof(val));
    }

    if ( progmeter_ ) progmeter_->setNrDone( ++nrdone_ );
    return Executor::MoreToDo();
}
