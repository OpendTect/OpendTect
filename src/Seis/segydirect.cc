/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.1 2008-08-06 12:08:56 cvsbert Exp $";

#include "segydirectdef.h"
#include "seistrc.h"
#include "segytr.h"
#include "posgeomdetector.h"
#include "datapointset.h"
#include "strmprov.h"
#include "dirlist.h"
#include "filepath.h"
#include "executor.h"


SEGYDirectDef::SEGYDirectDef( const char* fnm )
    : geomdtector_(*new PosGeomDetector(true))
{
    readFromFile( fnm );
}


SEGYDirectDef::SEGYDirectDef( Seis::GeomType gt, const IOPar& iop )
    : geom_(gt)
    , pars_(iop)
    , geomdtector_(*new PosGeomDetector(!Seis::is2D(gt)))
    , bpsample_(4)
    , segyformat_(1)
    , rev1_(true)
    , trcsz_(-1)
{
}


SEGYDirectDef::~SEGYDirectDef()
{
    deepErase( data_ );
    delete &geomdtector_;
}


bool SEGYDirectDef::readFromFile( const char* fnm )
{
    return false;
}


bool SEGYDirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	return false;
    return false;
}


struct SEGYDirectDefScanner : public Executor
{

SEGYDirectDefScanner( SEGYDirectDef* d, const char* dnm, const char* ge )
    : Executor("SEG-Y file scan")
    , def_(*d)
    , tr_(0)
    , curfidx_(-1)
    , curmsg_("Opening first file")
{
    FilePath fp( ge );
    DirList dl( dnm, DirList::FilesOnly, ge );
    for ( int idx=0; idx<dl.size(); idx++ )
	def_.fnms_.add( dl.fullPath(idx) );
}

void addFailed( int& fidx, const char* errmsg )
{
    def_.errmsgs_.add( errmsg );
    BufferString* bs = def_.fnms_[fidx];
    def_.failedfnms_ += bs;
    def_.fnms_ -= bs;
    fidx--;
}

int nextStep()
{
    return tr_ ? readNext() : openNext();
}

int readNext()
{
    if ( !tr_->read(trc_) )
    {
	if ( tr_->errMsg() && *tr_->errMsg() )
	    ErrMsg( tr_->errMsg() );
	delete tr_; tr_ = 0;
	def_.data_[curfidx_]->dataChanged();
	return Executor::MoreToDo;
    }

    DataPointSet::DataRow dr;
    dr.pos_.binid_ = trc_.info().binid;
    dr.pos_.nr_ = trc_.info().nr;
    dr.pos_.z_ = trc_.info().offset;
    def_.geomdtector_.add( dr.pos_.binid_, trc_.info().coord, dr.pos_.nr_ );
    def_.data_[curfidx_]->addRow( dr );
    return Executor::MoreToDo;
}

int openNext()
{
    curmsg_ = "Scanning";
    while ( true )
    {
	curfidx_++;
	if ( curfidx_ >= def_.fnms_.size() )
	{
	    if ( def_.fnms_.isEmpty() )
	    {
		curmsg_ = "No valid file found";
		return Executor::ErrorOccurred;
	    }
	    return Executor::Finished;
	}
	StreamData sd = StreamProvider( def_.fnms_.get(curfidx_) ).makeIStream();
	if ( !sd.usable() )
	{
	    sd.close();
	    addFailed( curfidx_, "Cannot open" );
	    continue;
	}

	tr_ = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
	tr_->usePar( def_.pars_ );
	if ( !tr_->initRead(new StreamConn(sd),Seis::Scan) )
	{
	    addFailed( curfidx_, tr_->errMsg() );
	    delete tr_; tr_ = 0;
	    continue;
	}

	if ( !initFileData() )
	    continue;

	return Executor::MoreToDo;
    }
}

bool initFileData()
{
    DataPointSet* dps = new DataPointSet( !Seis::is2D(def_.geom_), true );
    dps->setName( FilePath(def_.fnms_.get(curfidx_)).fileName() );

    if ( def_.trcsz_ < 0 )
    {
	def_.trcsz_ = tr_->inpNrSamples();
	def_.bpsample_ = tr_->dataBytes();
	def_.sampling_ = tr_->inpSD();
    }
    else if ( def_.trcsz_ != tr_->inpNrSamples() )
    {
	BufferString emsg( dps->name() );
	emsg += "Wrong #samples: "; tr_->inpNrSamples();
	emsg += "(should be "; emsg += def_.trcsz_; emsg += ")";
	addFailed( curfidx_, tr_->errMsg() );
	delete dps; delete tr_; tr_ = 0;
	return false;
    }

    def_.segyformat_ = tr_->numbfmt;
    def_.rev1_ = tr_->isRev1();
    def_.data_ += dps;
    return true;
}

const char* message() const	{ return curmsg_; }

    SEGYDirectDef&	def_;
    int			curfidx_;
    SEGYSeisTrcTranslator* tr_;
    SeisTrc		trc_;
    int			curtrcidx_;
    BufferString	curmsg_;

};


bool SEGYDirectDef::doScan( const char* dirnm, TaskRunner* tr, const char* ge )
{
    PtrMan<Executor> exec = new SEGYDirectDefScanner( this, dirnm, ge );
    return tr ? tr->execute( *exec ) : exec->execute();
}


Seis::Bounds* SEGYDirectDef::getBounds( const char* lnm ) const
{
    return 0;
}
