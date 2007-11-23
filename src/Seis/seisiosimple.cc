/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: seisiosimple.cc,v 1.5 2007-11-23 11:59:06 cvsbert Exp $";

#include "seisiosimple.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisimporter.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "oddirs.h"
#include "strmprov.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "scaler.h"
#include "msgh.h"

#include <math.h>

#define mStrmBinRead(var,typ) \
    sd_.istrm->read( (char*)(&var), sizeof(typ) )

#define mStrmBinWrite(var,typ) \
    sd_.ostrm->write( (const char*)(&var), sizeof(typ) )


SeisIOSimple::Data::Data( const char* filenm, Seis::GeomType gt )
    	: scaler_(0)
    	, resampler_(0)
	, subselpars_(*new IOPar("subsel"))
	, geom_(gt)
{
    clear(true);
    isasc_ = havepos_ = true;
    havenr_ = havesd_ = isxy_ = remnull_ = false;

    if ( filenm && *filenm )
	fname_ = filenm;
}


SeisIOSimple::Data::Data( const SeisIOSimple::Data& d )
    	: scaler_(0)
	, resampler_(0)
	, subselpars_(*new IOPar("subsel"))
{
    *this = d;
}


SeisIOSimple::Data& SeisIOSimple::Data::operator=( const SeisIOSimple::Data& d )
{
    if ( this == &d ) return *this;

    fname_ = d.fname_; seiskey_ = d.seiskey_;
    geom_ = d.geom_; isasc_ = d.isasc_;
    havesd_ = d.havesd_; sd_ = d.sd_;
    nrsamples_ = d.nrsamples_;
    havepos_ = d.havepos_; isxy_ = d.isxy_;
    havenr_ = d.havenr_; nrdef_ = d.nrdef_;
    inldef_ = d.inldef_; crldef_ = d.crldef_; nrcrlperinl_ = d.nrcrlperinl_;
    startpos_ = d.startpos_; steppos_ = d.steppos_;
    remnull_ = d.remnull_;
    setScaler( d.scaler_ );
    setResampler( d.resampler_ );
    subselpars_ = d.subselpars_;

    return *this;
}


SeisIOSimple::Data::~Data()
{
    delete scaler_;
    delete resampler_;
    delete &subselpars_;
}


void SeisIOSimple::Data::setScaler( Scaler* s )
{
    delete scaler_;
    scaler_ = s ? s->clone() : 0;
}


void SeisIOSimple::Data::setResampler( SeisResampler* r )
{
    delete resampler_;
    resampler_ = r ? new SeisResampler( *r ) : 0;
}


void SeisIOSimple::Data::clear( bool survchg )
{
    delete resampler_; resampler_ = 0;
    nrdef_.start = 1; nrdef_.step = 1;
    if ( !survchg )
	return;

    subselpars_.clear();
    fname_ = GetDataDir(); seiskey_ = "";
    sd_.start = (float)SI().zRange(false).start;
    sd_.step = (float)SI().zRange(false).step;
    nrsamples_ = SI().zRange(false).nrSteps() + 1;
    inldef_.start = SI().sampling(false).hrg.start.inl;
    crldef_.start = SI().sampling(false).hrg.start.crl;
    inldef_.step = SI().inlStep();
    crldef_.step = SI().crlStep();
    nrcrlperinl_ = (SI().sampling(false).hrg.stop.crl
		 - SI().sampling(false).hrg.start.crl)
		    / crldef_.step + 1;
    startpos_ = SI().transform( BinID(inldef_.start,crldef_.start) );
    Coord nextpos = SI().transform( BinID(inldef_.start+inldef_.step,
					  crldef_.start+crldef_.step) );
    steppos_.x = fabs( nextpos.x - startpos_.x );
    steppos_.y = fabs( nextpos.y - startpos_.y );
}


SeisIOSimple::SeisIOSimple( const Data& d, bool imp )
    	: Executor( imp ? "Import Seismics from simple file"
			: "Export Seismics to simple file" )
	, data_(d)
	, isimp_(imp)
    	, sd_(*new StreamData)
    	, trc_(*new SeisTrc)
    	, rdr_(0)
    	, wrr_(0)
    	, importer_(0)
    	, nrdone_(0)
    	, firsttrc_(true)
{
    PtrMan<IOObj> ioobj = IOM().get( data_.seiskey_ );
    if ( !ioobj ) return;

    SeisStoreAccess* sa;
    if ( isimp_ )
	sa = wrr_ = new SeisTrcWriter( ioobj );
    else
	sa = rdr_ = new SeisTrcReader( ioobj );
    errmsg_ = sa->errMsg();
    if ( !errmsg_.isEmpty() )
	return;

    Seis::SelData* seldata = Seis::SelData::get( data_.subselpars_ );
    sa->setSelData( seldata );

    StreamProvider sp( data_.fname_ );
    sd_ = isimp_ ? sp.makeIStream() : sp.makeOStream(); 
    if ( !sd_.usable() )
    {
	errmsg_ = isimp_ ? "Cannot open input file"
			 : "Cannot open output file";
	return;
    }

    if ( isimp_ )
	startImpRead();
}


SeisIOSimple::~SeisIOSimple()
{
    delete importer_;
    delete rdr_;
    delete wrr_;
    sd_.close();
    delete &sd_;
    delete &trc_;
}


class SeisIOSimpleImportReader : public SeisImporter::Reader
{
public:

SeisIOSimpleImportReader( SeisIOSimple& sios )
    	: sios_(sios)		{}

const char* name() const	{ return "Simple File"; }
bool fetch( SeisTrc& trc )
{
    int rv = sios_.readImpTrc( trc );
    if ( rv > 0 )
	{ sios_.nrdone_++; return true; }

    if ( rv < 0 )
	errmsg_ = sios_.message();
    return false;
}

    SeisIOSimple&	sios_;

};


void SeisIOSimple::startImpRead()
{
    if ( data_.havesd_ )
    {
	if ( data_.isasc_ )
	    *sd_.istrm >> data_.sd_.start >> data_.sd_.step
		       >> data_.nrsamples_;
	else
	{
	    mStrmBinRead( data_.sd_.start, float );
	    mStrmBinRead( data_.sd_.step, float );
	    mStrmBinRead( data_.nrsamples_, int );
	}
	if ( !sd_.istrm->good() )
	    { errmsg_ = "Input file contains no data"; return; }
	if ( SI().zIsTime() )
	    { data_.sd_.start *= .001; data_.sd_.step *= .001; }
    }

    trc_.info().sampling = data_.sd_;
    importer_ = new SeisImporter( new SeisIOSimpleImportReader(*this),
	    			  *wrr_, data_.geom_ );
}


const char* SeisIOSimple::message() const
{
    if ( importer_ )
	return importer_->message();

    return errmsg_.isEmpty() ? "Handling traces" : errmsg_.buf();
}


int SeisIOSimple::nrDone() const
{
    return importer_ ? importer_->nrDone() : nrdone_;
}


int SeisIOSimple::totalNr() const
{
    return importer_ ? importer_->totalNr() : -1;
}


const char* SeisIOSimple::nrDoneText() const
{
    return importer_ ? importer_->nrDoneText() : "Traces written";
}


int SeisIOSimple::nextStep()
{
    if ( !errmsg_.isEmpty() )
	return -1;
    else if ( isimp_ )
    {
	int rv = importer_ ? importer_->nextStep() : -1;
	if ( rv == Executor::Finished && importer_->nrSkipped() > 0 )
	    UsrMsg( BufferString("Warning: ",importer_->nrSkipped(),
				 " traces were rejected during import") );
	return rv;
    }

    int rv = readExpTrc();
    if ( rv < 0 )
	return errmsg_.isEmpty() ? Executor::Finished : Executor::ErrorOccurred;

    return rv == 0 ? Executor::MoreToDo : writeExpTrc();
}


int SeisIOSimple::readImpTrc( SeisTrc& trc )
{
    BinID bid; Coord coord; int nr;

    if ( !data_.havenr_ )
	nr = data_.nrdef_.start + nrdone_ * data_.nrdef_.step;
    else
    {
	if ( data_.isasc_ )
	    *sd_.istrm >> nr;
	else
	    mStrmBinRead( nr, int );
    }

    if ( !data_.havepos_ )
    {
	if ( data_.geom_ == Seis::Line )
	{
	    int nrinl = nrdone_ / data_.nrcrlperinl_;
	    int nrcrl = nrdone_ % data_.nrcrlperinl_;
	    coord.x = data_.startpos_.x + nrdone_ * data_.steppos_.x;
	    coord.y = data_.startpos_.y + nrdone_ * data_.steppos_.y;
	    bid = SI().transform( coord );
	}
	else
	{
	    int nrinl = nrdone_ / data_.nrcrlperinl_;
	    int nrcrl = nrdone_ % data_.nrcrlperinl_;
	    bid.inl = data_.inldef_.start + nrinl * data_.inldef_.step;
	    bid.crl = data_.crldef_.start + nrcrl * data_.crldef_.step;
	    coord = SI().transform( bid );
	}
    }
    else
    {
	if ( data_.isxy_ )
	{
	    if ( data_.isasc_ )
		*sd_.istrm >> coord.x >> coord.y;
	    else
	    {
		mStrmBinRead( coord.x, double );
		mStrmBinRead( coord.y, double );
	    }
	    bid = SI().transform( coord );
	}
	else
	{
	    if ( data_.isasc_ )
		*sd_.istrm >> bid.inl >> bid.crl;
	    else
	    {
		mStrmBinRead( bid.inl, int );
		mStrmBinRead( bid.crl, int );
	    }
	    coord = SI().transform( bid );
	}
    }

    trc.info() = trc_.info();
    trc.reSize( data_.nrsamples_, 0 );
    trc.info().binid = bid;
    trc.info().coord = coord;
    trc.info().nr = nr;
    float val;
    for ( int idx=0; idx<data_.nrsamples_; idx++ )
    {
	if ( data_.isasc_ )
	    *sd_.istrm >> val;
	else
	    mStrmBinRead( val, float );
	if ( data_.scaler_ ) val = data_.scaler_->scale( val );
	trc.set( idx, val, 0 );
    }

    return sd_.istrm->eof() ? Executor::Finished : Executor::MoreToDo;
}


int SeisIOSimple::readExpTrc()
{
    errmsg_ = "";
    int readres = rdr_->get( trc_.info() );
    if ( readres == 0 )
	return -2;
    else if ( readres > 1 )
	return 0;
    else if ( readres < 0 || !rdr_->get( trc_ ) )
    {
	errmsg_ = rdr_->errMsg();
	return -1;
    }

    if ( data_.resampler_ )
    {
	SeisTrc* curtrc = data_.resampler_->get( trc_ );
	if ( !curtrc )
	    return 0;
	if ( curtrc != &trc_ )
	    trc_ = *curtrc;
    }
    return 1;
}


int SeisIOSimple::writeExpTrc()
{
    if ( firsttrc_ )
    {
	firsttrc_ = false;
	data_.nrsamples_ = trc_.size();
	if ( data_.havesd_ )
	{
	    SamplingData<float> datasd = trc_.info().sampling;
	    if ( SI().zIsTime() )
		{ datasd.start *= 1000; datasd.step *= 1000; }
	    if ( data_.isasc_ )
		*sd_.ostrm << datasd.start << '\t'
			  << datasd.step << '\t'
			  << data_.nrsamples_ << std::endl;
	    else
	    {
		mStrmBinWrite( datasd.start, float );
		mStrmBinWrite( datasd.step, float );
		mStrmBinWrite( data_.nrsamples_, int );
	    }
	}
    }

    if ( data_.remnull_ && trc_.isNull() )
	return Executor::MoreToDo;

    if ( data_.havenr_ )
    {
	const int nr = trc_.info().nr;
	if ( data_.isasc_ )
	{
	    *sd_.ostrm << nr;
	    if ( data_.havepos_ )
		*sd_.ostrm << ' ';
	}
	else
	    mStrmBinWrite( nr, int );
    }

    if ( data_.havepos_ )
    {
	if ( data_.isxy_ )
	{
	    const Coord coord = trc_.info().coord;
	    if ( data_.isasc_ )
	    {
		*sd_.ostrm << getStringFromDouble(0,coord.x) << ' ';
		*sd_.ostrm << getStringFromDouble(0,coord.y);
	    }
	    else
	    {
		mStrmBinWrite( coord.x, double );
		mStrmBinWrite( coord.y, double );
	    }
	}
	else
	{
	    const BinID bid = trc_.info().binid;
	    if ( data_.isasc_ )
		*sd_.ostrm << bid.inl << '\t' << bid.crl;
	    else
	    {
		mStrmBinWrite( bid.inl, int );
		mStrmBinWrite( bid.crl, int );
	    }
	}
    }

    float val;
    for ( int idx=0; idx<data_.nrsamples_; idx++ )
    {
	val = trc_.get( idx, 0 );
	if ( data_.scaler_ ) val = data_.scaler_->scale( val );
	if ( data_.isasc_ )
	    *sd_.ostrm << '\t' << val;
	else
	    mStrmBinWrite( val, float );
    }

    if ( data_.isasc_ )
	*sd_.ostrm << std::endl;

    if ( !sd_.ostrm->good() )
	{ errmsg_ = "Error during write"; return Executor::ErrorOccurred; }

    nrdone_++;
    return Executor::MoreToDo;
}
