/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: segyscanner.cc,v 1.4 2008-10-02 14:40:06 cvsbert Exp $";

#include "segyscanner.h"
#include "segyfiledef.h"
#include "segyhdr.h"
#include "seistrc.h"
#include "segytr.h"
#include "datapointset.h"
#include "strmprov.h"
#include "dirlist.h"
#include "filepath.h"
#include "executor.h"
#include "iopar.h"

#define mDefMembInit \
      Executor("SEG-Y file scan") \
    , trc_(*new SeisTrc) \
    , pars_(*new IOPar(i)) \
    , tr_(0) \
    , geom_(gt) \
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
}


SEGY::Scanner::~Scanner()
{
    deepErase( fd_ );
    delete &trc_;
    delete const_cast<IOPar*>(&pars_);
}


void SEGY::Scanner::getReport( IOPar& iop ) const
{
    const bool is2d = Seis::is2D( geom_ );
    const bool isps = Seis::isPS( geom_ );
    iop.set( "->", "Provided information" );
    FileSpec fs; fs.usePar( pars_ ); fs.getReport( iop );
    FilePars fp(true); fp.usePar( pars_ ); fp.getReport( iop );

    if ( fd_.isEmpty() )
    {
	if ( failedfnms_.isEmpty() )
	    iop.set( "->", "No matching files found" );
	else
	    addErrReport( iop );
	return;
    }

    bool forecrev0 = false;
    pars_.getYN( SEGYSeisTrcTranslator::sForceRev0, forecrev0 );

    int icopt = 0;
    pars_.get( SegylikeSeisTrcTranslator::sKeyIC2XYOpt, icopt );

    addErrReport( iop );
}


void SEGY::Scanner::addErrReport( IOPar& iop ) const
{
}


int SEGY::Scanner::nextStep()
{
    if ( nrtrcs_ > 0 && nrdone_ >= nrtrcs_ )
	return Executor::Finished;
    return tr_ ? readNext() : openNext();
}


int SEGY::Scanner::readNext()
{
    SEGY::FileData& fd = *fd_[curfidx_];
    if ( !tr_->read(trc_) )
    {
	fd.addEnded();

	const char* emsg = tr_->errMsg();
	if ( emsg && *emsg )
	{
	    scanerrfnms_.add( fnms_.get(curfidx_) );
	    scanerrmsgs_.add( emsg );
	}
	delete tr_; tr_ = 0;
	return Executor::MoreToDo;
    }

    fd.add( trc_.info().binid, trc_.info().coord, trc_.info().nr,
	    trc_.info().offset, trc_.isNull(), tr_->trcHeader().isusable );
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
	    return Executor::ErrorOccurred;
	}
	return Executor::Finished;
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
    if ( !tr_->initRead(new StreamConn(sd),Seis::Scan) )
    {
	addFailed( tr_->errMsg() );
	delete tr_; tr_ = 0;
	return Executor::MoreToDo;
    }

    initFileData();
    return Executor::MoreToDo;
}


void SEGY::Scanner::addFailed( const char* errmsg )
{
    failerrmsgs_.add( errmsg );
    BufferString* bs = fnms_[curfidx_];
    failedfnms_ += bs;
    fnms_ -= bs;
    curfidx_--;
}


void SEGY::Scanner::initFileData()
{
    FileData* newfd = new FileData( fnms_.get(curfidx_), geom_ );
    if ( !fd_.isEmpty() && tr_->inpNrSamples() != fd_[0]->trcsz_ )
    {
	BufferString emsg( "Wrong #samples: " ); tr_->inpNrSamples();
	emsg += "(should be "; emsg += fd_[0]->trcsz_; emsg += ")";
	addFailed( tr_->errMsg() );
	delete newfd; delete tr_; tr_ = 0;
	return;
    }

    newfd->trcsz_ = tr_->inpNrSamples();
    newfd->sampling_ = tr_->inpSD();
    newfd->segyfmt_ = tr_->numbfmt;
    newfd->isrev1_ = tr_->isRev1();
    newfd->nrstanzas_ = tr_->binHeader().nrstzs;

    fd_ += newfd;
}


bool SEGY::Scanner::toNext( SEGY::TrcFileIdx& tfi ) const
{
    if ( tfi.filenr_ < 0 )
	{ tfi.trcnr_ = -1; tfi.filenr_= 0; }

    if ( fd_.isEmpty() || tfi.filenr_ >= fd_.size() )
	{ tfi.filenr_ = -1; tfi.trcnr_ = 0; return false; }

    tfi.trcnr_++;
    if ( tfi.trcnr_ >= fd_[tfi.filenr_]->nrTraces() )
	{ tfi.filenr_++; tfi.trcnr_ = -1; return toNext( tfi ); }

    return true;
}
