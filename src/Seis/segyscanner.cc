/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2008
-*/
static const char* rcsID = "$Id: segyscanner.cc,v 1.33 2012/03/07 12:44:01 cvsbert Exp $";

#include "segyscanner.h"

#include "oddirs.h"
#include "segyfiledata.h"
#include "segyhdr.h"
#include "seistrc.h"
#include "segytr.h"
#include "survinfo.h"
#include "datapointset.h"
#include "posinfodetector.h"
#include "strmprov.h"
#include "dirlist.h"
#include "dataclipper.h"
#include "filepath.h"
#include "executor.h"
#include "iopar.h"

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
    , msg_("Opening first file") \
    , richinfo_(false) \
    , nrdone_(0) \
    , notrcinfo_(false) \
    , totnr_(-2) \
    , nrtrcs_(0)

SEGY::Scanner::Scanner( const FileSpec& fs, Seis::GeomType gt, const IOPar& i )
    : mDefMembInit
{
    init( fs );
}


SEGY::Scanner::Scanner( const IOPar& i, Seis::GeomType gt )
    : mDefMembInit
{
    FileSpec fs; fs.usePar( pars_ );
    init( fs );
}


void SEGY::Scanner::init( const FileSpec& fs )
{
    const int nrfiles = fs.nrFiles();
    for ( int idx=0; idx<nrfiles; idx++ )
	fnms_.add( fs.getFileName(idx) );
    pars_.getYN( FileDef::sKeyForceRev0(), forcerev0_ );
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
    if ( !tr_ ) return;
    trwarns_.add( tr_->warnings(), true );
    delete tr_; tr_ = 0;
}


void SEGY::Scanner::getReport( IOPar& iop ) const
{
    const bool isrev1 = !forcerev0_ && (fds_.isEmpty() || fds_.isRev1() );

    iop.add( IOPar::sKeyHdr(), "Provided information" );
    FileSpec fs; fs.usePar( pars_ ); fs.getReport( iop, isrev1 );
    FilePars fp(true); fp.usePar( pars_ ); fp.getReport( iop, isrev1 );
    FileReadOpts fro(geom_); fro.usePar( pars_ ); fro.getReport( iop, isrev1 );

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
	if ( scanerrfnms_.indexOf(fnm) < 0 )
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
	    iop.add( keyw, scanerrmsgs_.get(idx) );
	}
    }

    for ( int idx=0; idx<failedfnms_.size(); idx++ )
    {
	BufferString keyw( "Failed to read " );
	keyw += failedfnms_.get( idx );
	iop.add( keyw, failerrmsgs_.get(idx) );
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
	const char* emsg = tr_->errMsg();
	if ( emsg && *emsg )
	{
	    scanerrfnms_.add( fnms_.get(curfidx_) );
	    scanerrmsgs_.add( emsg );
	}
	closeTr();
	return Executor::MoreToDo();
    }

    const SeisTrcInfo& ti = trc_.info();
    dtctor_.add( ti.coord, ti.binid, ti.nr, ti.offset );
    clipsmplr_.add( (const float*)trc_.data().getComponent(0)->data(),
	    	    trc_.size() );
    nrdone_++;

    if ( notrcinfo_ )
	return Executor::MoreToDo();

    fds_.addTrace( curfidx_, ti.posKey( geom_ ), ti.coord,
	    	   tr_->trcHeader().isusable );

    return Executor::MoreToDo();
}


int SEGY::Scanner::openNext()
{
    msg_ = "Scanning";
    curfidx_++;
    if ( curfidx_ >= fnms_.size() )
    {
	if ( fnms_.isEmpty() )
	{
	    msg_ = "No valid file found";
	    return finish( false );
	}
	return finish( true );
    }

    BufferString path = fnms_.get(curfidx_);
#ifdef __win__
    replaceCharacter( path.buf(), '/', '\\' );  
#endif
    FilePath fp( path );
    if ( !fp.isAbsolute() )
	fp.insert( GetDataDir() );
    BufferString abspath = fp.fullPath();
    StreamData sd = StreamProvider( abspath ).makeIStream();
    
    if ( !sd.usable() )
	{ addFailed( "Cannot open this file" ); return Executor::MoreToDo(); }

    tr_ = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
    tr_->usePar( pars_ );
    tr_->setForceRev0( forcerev0_ );
    if ( !tr_->initRead(new StreamConn(sd),Seis::Scan) )
    {
	addFailed( tr_->errMsg() );
	return Executor::MoreToDo();
    }
    for ( int idx=0; idx<tr_->componentInfo().size(); idx++ )
	tr_->componentInfo()[idx]->datachar
	    = DataCharacteristics( DataCharacteristics::F32 );

    initFileData();
    return Executor::MoreToDo();
}


int SEGY::Scanner::finish( bool allok )
{
    dtctor_.finish();
    return allok ? Executor::Finished() : Executor::ErrorOccurred();
}


void SEGY::Scanner::addFailed( const char* errmsg )
{
    failerrmsgs_.add( errmsg );
    BufferString* bs = fnms_[curfidx_];
    failedfnms_ += new BufferString( *bs );
    fnms_ -= bs;
    curfidx_--;
    closeTr();
}


StepInterval<float> SEGY::Scanner::zRange() const
{
    if ( fds_.isEmpty() )
	return SI().zRange( false );

    StepInterval<float> ret( fds_.getSampling().interval(fds_.getTrcSz()) );

    return ret;
}


void SEGY::Scanner::initFileData()
{
    if ( !curfidx_ )
    {
	fds_.setAuxData( geom_, *tr_ );
    }
    else if ( tr_->inpNrSamples() != fds_.getTrcSz() )
    {
	BufferString emsg( "Wrong #samples: " ); tr_->inpNrSamples();
	emsg += "(must be same as first file: ";
	emsg += fds_.getTrcSz(); emsg += ")";
	addFailed( tr_->errMsg() );
	return;
    }
/*
    newfd->trcsz_ = tr_->inpNrSamples();
    newfd->sampling_ = tr_->inpSD();
    newfd->segyfmt_ = tr_->filePars().fmt_;
    newfd->isrev1_ = tr_->isRev1();
    newfd->nrstanzas_ = tr_->binHeader().nrstzs;
    */
    fds_.addFile( fnms_.get(curfidx_) );
}
