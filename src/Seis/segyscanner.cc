/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2008
-*/

#include "segyscanner.h"

#include "oddirs.h"
#include "segyfiledata.h"
#include "segyhdr.h"
#include "seistrc.h"
#include "segytr.h"
#include "survinfo.h"
#include "datapointset.h"
#include "posinfodetector.h"
#include "dirlist.h"
#include "od_istream.h"
#include "dataclipper.h"
#include "filepath.h"
#include "executor.h"
#include "iopar.h"
#include "uistrings.h"

#define mDefMembInit \
      Executor("SEG-Y file scan") \
    , trc_(*new SeisTrc) \
    , pars_(*new IOPar(i)) \
    , fds_(*new FileDataSet(i)) \
    , dtctor_(*new PosInfo::Detector( PosInfo::Detector::Setup(Seis::is2D(gt)) \
			.isps(Seis::isPS(gt)).reqsorting(true) ) ) \
    , tr_(0) \
    , clipsmplr_(*new DataClipSampler) \
    , geom_(gt) \
    , forcerev0_(false) \
    , curfidx_(-1) \
    , msg_(Openff()) \
    , richinfo_(false) \
    , nrdone_(0) \
    , notrcinfo_(false) \
    , totnr_(-2) \
    , nrtrcs_(0)

SEGY::Scanner::Scanner( const FileSpec& fs, Seis::GeomType gt, const IOPar& i )
    : mDefMembInit
{
    filespec_ = fs;
    init();
}


SEGY::Scanner::Scanner( const IOPar& i, Seis::GeomType gt )
    : mDefMembInit
{
    filespec_.usePar( pars_ );
    init();
}


void SEGY::Scanner::init()
{
    const int nrfiles = filespec_.nrFiles();
    for ( int idx=0; idx<nrfiles; idx++ )
	fnms_.add( filespec_.fileName(idx) );
    pars_.getYN( FilePars::sKeyForceRev0(), forcerev0_ );
}


SEGY::Scanner::~Scanner()
{
    closeTr();
    delete &fds_;
    delete &trc_;
    delete &clipsmplr_;
    delete const_cast<IOPar*>(&pars_);
    delete &dtctor_;
}


void SEGY::Scanner::closeTr()
{
    if ( !tr_ )
	return;
    trwarns_.add( tr_->warnings() );
    delete tr_; tr_ = 0;
}


uiRetVal SEGY::Scanner::fillStats( IOPar& iop ) const
{
    if ( !seisstatscollector_.fillPar(iop) )
	return tr("Not enough different traces to calculate stats");

    return uiRetVal::OK();
}


void SEGY::Scanner::getReport( IOPar& iop, const IOPar* inppars ) const
{
    const bool isrev0 = forcerev0_ || fds_.isEmpty() || fds_.isRev0();

    if ( !inppars )
	inppars = &pars_;
    iop.add( IOPar::sKeyHdr(), "Provided information" );
    FileSpec fs; fs.usePar( *inppars ); fs.getReport( iop );
    FilePars fp(true); fp.usePar( *inppars ); fp.getReport( iop, isrev0 );
    FileReadOpts fro(geom_); fro.usePar( *inppars ); fro.getReport(iop,isrev0);

    if ( fds_.isEmpty() )
    {
	if ( failedfnms_.isEmpty() )
	    iop.add( IOPar::sKeySubHdr(), "No matching files found" );
	else
	    addErrReport( iop );
	return;
    }

    iop.add( IOPar::sKeyHdr(), "Position scanning results" );
    dtctor_.report( iop );
    if ( richinfo_ )
    {
	iop.add( IOPar::sKeyHdr(), "Clipping data" );
	clipsmplr_.report( iop );
    }
    addErrReport( iop );

    fds_.getReport( iop );
}


void SEGY::Scanner::addErrReport( IOPar& iop ) const
{
    iop.add( IOPar::sKeyHdr(),  "Status" );
    for ( int idx=0; idx<fnms_.size(); idx++ )
    {
	const char* fnm = fnms_.get( idx );
	if ( !scanerrfnms_.isPresent(fnm) )
	{
	    if ( idx < fds_.nrFiles() )
		iop.add( "Successfully scanned", fnm );
	    else
		iop.add( "Not scanned", fnm );
	}
	else
	{
	    BufferString keyw( "Error during read of " );
	    keyw += fnm;
	    iop.add( keyw, toString(scanerrmsgs_[idx]) );
	}
    }

    for ( int idx=0; idx<failedfnms_.size(); idx++ )
    {
	BufferString keyw( "Failed to read " );
	keyw += failedfnms_.get( idx );
	iop.add( keyw, toString(failerrmsgs_[idx]) );
    }

}


od_int64 SEGY::Scanner::totalNr() const
{
    if ( totnr_ == -2 )
    {
	if ( !tr_ ) return -1;
	totnr_ = tr_->estimatedNrTraces();
	totnr_ *= fnms_.size();
    }
    return totnr_;
}


int SEGY::Scanner::nextStep()
{
    if ( nrtrcs_ > 0 && nrdone_ >= nrtrcs_ )
	return finish( true );
    return tr_ ? readNext() : openNext();
}


int SEGY::Scanner::readNext()
{
    if ( curfidx_ < 0 || curfidx_ >= fds_.nrFiles() )
	return finish( true );

    if ( !tr_->read(trc_) )
    {
	const uiString emsg = tr_->errMsg();
	if ( !emsg.isEmpty() )
	{
	    scanerrfnms_.add( fnms_.get(curfidx_) );
	    scanerrmsgs_.add( emsg );
	}
	closeTr();
	return Executor::MoreToDo();
    }

    const SeisTrcInfo& ti = trc_.info();
    dtctor_.add( ti.coord_, ti.binID(), ti.trcNr(), ti.offset_ );
    clipsmplr_.add( (const float*)trc_.data().getComponent(0)->data(),
		    trc_.size() );
    seisstatscollector_.useTrace( trc_ );
    nrdone_++;

    if ( notrcinfo_ )
	return Executor::MoreToDo();

    fds_.addTrace( curfidx_, ti.posKey( geom_ ), ti.coord_,
		   tr_->trcHeader().isusable );

    return Executor::MoreToDo();
}


int SEGY::Scanner::openNext()
{
    msg_ = uiStrings::sScanning();
    curfidx_++;
    if ( curfidx_ >= fnms_.size() )
    {
	if ( fnms_.isEmpty() )
	{
	    msg_ = tr("No valid file found");
	    return finish( false );
	}
	return finish( true );
    }

    BufferString path = fnms_.get(curfidx_);
#ifdef __win__
    path.replace( '/', '\\' );
#endif
    File::Path fp( path );
    if ( !fp.isAbsolute() )
	fp.insert( GetDataDir() );
    BufferString abspath = fp.fullPath();

    od_istream* strm = new od_istream( abspath );
    if ( !strm || !strm->isOK() )
	{ delete strm; addFailed( tr("Cannot open this file") );
		return Executor::MoreToDo(); }

    tr_ = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
    tr_->usePar( pars_ );
    tr_->setForceRev0( forcerev0_ );
    if ( !tr_->initRead(new StreamConn(strm),Seis::Scan) )
    {
	addFailed( tr_->errMsg() );
	return Executor::MoreToDo();
    }
    for ( int idx=0; idx<tr_->componentInfo().size(); idx++ )
	tr_->componentInfo()[idx]->datachar_ = DataCharacteristics( OD::F32 );

    initFileData();
    return Executor::MoreToDo();
}


int SEGY::Scanner::finish( bool allok )
{
    dtctor_.finish();
    return allok ? Executor::Finished() : Executor::ErrorOccurred();
}


void SEGY::Scanner::addFailed( const uiString& errmsg )
{
    failerrmsgs_.add( errmsg );
    failedfnms_.add( fnms_.get(curfidx_) );
    fnms_.removeSingle( curfidx_ );
    curfidx_--;
    closeTr();
}


StepInterval<float> SEGY::Scanner::zRange() const
{
    if ( fds_.isEmpty() )
	return SI().zRange();

    return StepInterval<float>( fds_.getSampling().interval(fds_.getTrcSz()) );
}


void SEGY::Scanner::initFileData()
{
    if ( curfidx_ == 0 )
	fds_.setAuxData( geom_, *tr_ );
    else if ( tr_->inpNrSamples() != fds_.getTrcSz() )
    {
	uiString emsg = tr("Wrong #samples: %1"
			   "(must be same as first file: %2)")
		      .arg(tr_->inpNrSamples())
		      .arg(fds_.getTrcSz());
	addFailed( tr_->errMsg() );
	return;
    }

    fds_.addFile( fnms_.get(curfidx_) );
}
