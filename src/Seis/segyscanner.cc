/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: segyscanner.cc,v 1.1 2008-09-18 14:55:52 cvsbert Exp $";

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


SEGY::Scanner::Scanner( const FileSpec& fs, Seis::GeomType gt, const IOPar& i )
    : Executor("SEG-Y file scan")
    , trc_(*new SeisTrc)
    , pars_(*new IOPar(i))
    , geom_(gt)
    , curfidx_(-1)
    , msg_("Opening first file")
    , nrdone_(0)
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


int SEGY::Scanner::nextStep()
{
    return tr_ ? readNext() : openNext();
}


int SEGY::Scanner::readNext()
{
    SEGY::FileData& fd = *fd_[curfidx_];
    if ( !tr_->read(trc_) )
    {
	fd.data_.dataChanged();

	const char* emsg = tr_->errMsg();
	if ( !emsg || !*emsg )
	    emsg = "Unknown error during read";
	scanerrfnms_.add( fnms_.get(curfidx_) );
	scanerrmsgs_.add( emsg );
	delete tr_; tr_ = 0;
	return Executor::MoreToDo;
    }

    DataPointSet::DataRow dr;
    dr.pos_.binid_ = trc_.info().binid;
    dr.pos_.nr_ = trc_.info().nr;
    dr.pos_.z_ = trc_.info().offset;
    dr.setSel( trc_.isNull() );
    dr.setGroup( tr_->trcHeader().isusable ? 1 : 2 );
    fd.data_.addRow( dr );
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
