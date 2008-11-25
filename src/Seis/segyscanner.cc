/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: segyscanner.cc,v 1.15 2008-11-25 11:37:46 cvsbert Exp $";

#include "segyscanner.h"
#include "segyfiledata.h"
#include "segyhdr.h"
#include "seistrc.h"
#include "segytr.h"
#include "survinfo.h"
#include "datapointset.h"
#include "posinfodetector.h"
#include "strmprov.h"
#include "dirlist.h"
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
    , geom_(gt) \
    , forcerev0_(false) \
    , curfidx_(-1) \
    , msg_("Opening first file") \
    , nrdone_(0) \
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
    pars_.getYN( FileDef::sKeyForceRev0, forcerev0_ );
}


SEGY::Scanner::~Scanner()
{
    closeTr();
    delete &fds_;
    delete &trc_;
    delete const_cast<IOPar*>(&pars_);
}


void SEGY::Scanner::closeTr()
{
    if ( !tr_ ) return;
    trwarns_.add( tr_->warnings(), true );
    delete tr_; tr_ = 0;
}


void SEGY::Scanner::getReport( IOPar& iop ) const
{
    const bool isrev1 = !forcerev0_ && (fds_.isEmpty() || fds_[0]->isrev1_);

    iop.add( "->", "Provided information" );
    FileSpec fs; fs.usePar( pars_ ); fs.getReport( iop, isrev1 );
    FilePars fp(true); fp.usePar( pars_ ); fp.getReport( iop, isrev1 );
    FileReadOpts fro(geom_); fro.usePar( pars_ ); fro.getReport( iop, isrev1 );

    if ( fds_.isEmpty() )
    {
	if ( failedfnms_.isEmpty() )
	    iop.add( "->", "No matching files found" );
	else
	    addErrReport( iop );
	return;
    }

    iop.add( "->", "Position scanning results" );
    dtctor_.report( iop );
    addErrReport( iop );

    for ( int idx=0; idx<fds_.size(); idx++ )
	fds_[idx]->getReport( iop );
}


void SEGY::Scanner::addErrReport( IOPar& iop ) const
{
    iop.add( "->",  "Status" );
    for ( int idx=0; idx<fnms_.size(); idx++ )
    {
	const char* fnm = fnms_.get( idx );
	if ( scanerrfnms_.indexOf(fnm) < 0 )
	{
	    if ( idx < fds_.size() )
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


int SEGY::Scanner::nextStep()
{
    if ( nrtrcs_ > 0 && nrdone_ >= nrtrcs_ )
	return finish( true );
    return tr_ ? readNext() : openNext();
}


int SEGY::Scanner::readNext()
{
    SEGY::FileData& fd = *fds_[curfidx_];
    if ( !tr_->read(trc_) )
    {
	const char* emsg = tr_->errMsg();
	if ( emsg && *emsg )
	{
	    scanerrfnms_.add( fnms_.get(curfidx_) );
	    scanerrmsgs_.add( emsg );
	}
	closeTr();
	return Executor::MoreToDo;
    }

    const SeisTrcInfo& ti = trc_.info();
    dtctor_.add( ti.coord, ti.binid, ti.nr, ti.offset );

    SEGY::TraceInfo sgyti( geom_ );
    sgyti.pos_.set( ti.nr, ti.binid, ti.offset );
    sgyti.coord_ = ti.coord;
    sgyti.null_ = trc_.isNull();
    sgyti.usable_ = tr_->trcHeader().isusable;
    fd += sgyti;

    nrdone_++;
    return Executor::MoreToDo;
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

    StreamData sd = StreamProvider(fnms_.get(curfidx_)).makeIStream();
    if ( !sd.usable() )
    {
	sd.close();
	addFailed( "Cannot open this file" );
	return Executor::MoreToDo;
    }

    tr_ = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
    tr_->usePar( pars_ );
    tr_->setForceRev0( forcerev0_ );
    if ( !tr_->initRead(new StreamConn(sd),Seis::Scan) )
    {
	addFailed( tr_->errMsg() );
	closeTr();
	return Executor::MoreToDo;
    }

    initFileData();
    return Executor::MoreToDo;
}


int SEGY::Scanner::finish( bool allok )
{
    dtctor_.finish();
    return allok ? Executor::Finished : Executor::ErrorOccurred;
}


void SEGY::Scanner::addFailed( const char* errmsg )
{
    failerrmsgs_.add( errmsg );
    BufferString* bs = fnms_[curfidx_];
    failedfnms_ += bs;
    fnms_ -= bs;
    curfidx_--;
}


StepInterval<float> SEGY::Scanner::zRange() const
{
    if ( fds_.isEmpty() )
	return SI().zRange( false );

    const FileData& fd = *fds_[0];
    StepInterval<float> ret;
    ret.start = fd.sampling_.start;
    ret.step = fd.sampling_.step;
    ret.stop = ret.start + (fd.trcsz_-1) * ret.step;

    return ret;
}


void SEGY::Scanner::initFileData()
{
    FileData* newfd = new FileData( fnms_.get(curfidx_), geom_ );
    if ( !fds_.isEmpty() && tr_->inpNrSamples() != fds_[0]->trcsz_ )
    {
	BufferString emsg( "Wrong #samples: " ); tr_->inpNrSamples();
	emsg += "(must be same as first file: ";
	emsg += fds_[0]->trcsz_; emsg += ")";
	addFailed( tr_->errMsg() );
	delete newfd; closeTr();
	return;
    }

    newfd->trcsz_ = tr_->inpNrSamples();
    newfd->sampling_ = tr_->inpSD();
    newfd->segyfmt_ = tr_->filePars().fmt_;
    newfd->isrev1_ = tr_->isRev1();
    newfd->nrstanzas_ = tr_->binHeader().nrstzs;

    fds_ += newfd;
}
