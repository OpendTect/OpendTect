/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: seisiosimple.cc,v 1.26 2012/07/10 13:06:03 cvskris Exp $";

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
#include "strmoper.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "scaler.h"
#include "keystrs.h"
#include "linekey.h"
#include "posimpexppars.h"
#include "zdomain.h"
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
	, linekey_(*new LineKey)
	, geom_(gt)
	, compidx_(0)
{
    clear(true);
    isasc_ = havepos_ = true;
    havenr_ = haverefnr_ = havesd_ = haveoffs_ = haveazim_
	    = isxy_ = remnull_ = false;

    if ( filenm && *filenm )
	fname_ = filenm;
}


SeisIOSimple::Data::Data( const SeisIOSimple::Data& d )
    	: scaler_(0)
	, resampler_(0)
	, subselpars_(*new IOPar("subsel"))
	, linekey_(*new LineKey)
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
    havenr_ = d.havenr_; haverefnr_ = d.haverefnr_; nrdef_ = d.nrdef_;
    haveoffs_ = d.haveoffs_; offsdef_ = d.offsdef_; haveazim_ = d.haveazim_;
    nroffsperpos_ = d.nroffsperpos_;
    inldef_ = d.inldef_; crldef_ = d.crldef_; nrcrlperinl_ = d.nrcrlperinl_;
    startpos_ = d.startpos_; steppos_ = d.steppos_;
    remnull_ = d.remnull_;
    setScaler( d.scaler_ );
    setResampler( d.resampler_ );
    linekey_ = d.linekey_;
    subselpars_ = d.subselpars_;
    compidx_ = d.compidx_;

    return *this;
}


SeisIOSimple::Data::~Data()
{
    delete scaler_;
    delete resampler_;
    delete &linekey_;
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

    subselpars_.setEmpty();
    fname_ = GetDataDir(); seiskey_ = ""; linekey_ = "";
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
    nroffsperpos_ = 10; // cannot think of a good default ...
    startpos_ = SI().transform( BinID(inldef_.start,crldef_.start) );
    Coord nextpos = SI().transform( BinID(inldef_.start+inldef_.step,
					  crldef_.start+crldef_.step) );
    steppos_.x = fabs( nextpos.x - startpos_.x );
    steppos_.y = fabs( nextpos.y - startpos_.y );
    offsdef_.start = 0; offsdef_.step = SI().crlDistance();
    compidx_ = 0;
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
	, offsnr_(0)
	, prevbid_(mUdf(int),0)
	, prevnr_(mUdf(int))
	, zistm_(SI().zIsTime())
{
    PtrMan<IOObj> ioobj = IOM().get( data_.seiskey_ );
    if ( !ioobj ) return;
    zistm_ = ZDomain::isTime( ioobj->pars() );

    SeisStoreAccess* sa;
    if ( isimp_ )
	sa = wrr_ = new SeisTrcWriter( ioobj );
    else
	sa = rdr_ = new SeisTrcReader( ioobj );
    errmsg_ = sa->errMsg();
    if ( !errmsg_.isEmpty() )
	return;

    if ( !data_.linekey_.isEmpty() )
    {
	data_.subselpars_.set( sKey::LineKey, data_.linekey_ );
	data_.subselpars_.set( sKey::Attribute, data_.linekey_.attrName());
	    // Needed because attrnm can disappear from line key
    }
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
	if ( zistm_ )
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


od_int64 SeisIOSimple::nrDone() const
{
    return importer_ ? importer_->nrDone() : nrdone_;
}


od_int64 SeisIOSimple::totalNr() const
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
	if ( rv == Executor::Finished() && importer_->nrSkipped() > 0 )
	    UsrMsg( BufferString("Warning: ",importer_->nrSkipped(),
				 " traces were rejected during import") );
	return rv;
    }

    int rv = readExpTrc();
    if ( rv < 0 )
	return errmsg_.isEmpty()
	    ? Executor::Finished()
	    : Executor::ErrorOccurred();

    return rv == 0 ? Executor::MoreToDo() : writeExpTrc();
}


int SeisIOSimple::readImpTrc( SeisTrc& trc )
{
    std::istream& strm = *sd_.istrm;
    if ( !strm.good() )
	return Executor::Finished();

    BinID bid; Coord coord; int nr = 1; float offs = 0, azim = 0, refnr = 0;
    const bool is2d = Seis::is2D(data_.geom_);
    const bool isps = Seis::isPS(data_.geom_);

    if ( !data_.havenr_ )
	nr = data_.nrdef_.start + nrdone_ * data_.nrdef_.step;
    else
    {
	if ( data_.isasc_ )
	{
	    strm >> coord.x;
	    nr = mNINT32(coord.x);
	    if ( data_.haverefnr_ )
		strm >> refnr;
	}
	else
	{
	    mStrmBinRead( nr, int );
	    if ( data_.haverefnr_ )
		mStrmBinRead( refnr, float );
	}
    }

    if ( !data_.havepos_ )
    {
	const int nrposdone = isps ? nrdone_ / data_.nroffsperpos_ : nrdone_;
	if ( is2d )
	{
	    coord.x = data_.startpos_.x + nrposdone * data_.steppos_.x;
	    coord.y = data_.startpos_.y + nrposdone * data_.steppos_.y;
	    bid = SI().transform( coord );
	}
	else
	{
	    const int nrinl = nrposdone / data_.nrcrlperinl_;
	    const int nrcrl = nrposdone % data_.nrcrlperinl_;
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
		strm >> coord.x >> coord.y;
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
	    {
		strm >> coord.x >> coord.y;
		bid.inl = mNINT32(coord.x); bid.crl = mNINT32(coord.y);
	    }
	    else
	    {
		mStrmBinRead( bid.inl, int );
		mStrmBinRead( bid.crl, int );
	    }
	    coord = SI().transform( bid );
	}

    }

    if ( isps )
    {
	if ( !data_.haveoffs_ )
	    offs = data_.offsdef_.start + offsnr_ * data_.offsdef_.step;
	else
	{
	    if ( data_.isasc_ )
		strm >> offs;
	    else
		mStrmBinRead( offs, float );
	}
	if ( data_.haveazim_ )
	{
	    if ( data_.isasc_ )
		strm >> azim;
	    else
		mStrmBinRead( azim, float );
	}
	if ( (is2d && nr != prevnr_) || (!is2d && bid != prevbid_) )
	    offsnr_ = 0;
	else
	    offsnr_++;
    }

    if ( !strm.good() )
	return Executor::Finished();

    mPIEPAdj(BinID,bid,true); mPIEPAdj(Coord,coord,true);
    mPIEPAdj(TrcNr,nr,true); mPIEPAdj(Offset,offs,true);
    trc.info() = trc_.info();
    trc.reSize( data_.nrsamples_, 0 );
    trc.info().binid = bid;
    prevbid_ = bid;
    trc.info().coord = coord;
    trc.info().offset = SI().xyInFeet() ? offs * mFromFeetFactor : offs;
    trc.info().azimuth = azim;
    trc.info().nr = nr;
    trc.info().refnr = refnr;
    prevnr_ = nr;
    float val; char buf[128]; const char* ptr = buf;
    for ( int idx=0; idx<data_.nrsamples_; idx++ )
    {
	if ( data_.isasc_ )
	{
	    if ( !StrmOper::wordFromLine( strm, buf, 127 ) )
		return Executor::Finished();
	    Conv::set( val, ptr );
	}
	else
	{
	    mStrmBinRead( val, float );
	    if ( !strm.good() )
		return Executor::Finished();
	}

	if ( data_.scaler_ ) val = data_.scaler_->scale( val );
	trc.set( idx, val, 0 );
    }

    return Executor::MoreToDo();
}


int SeisIOSimple::readExpTrc()
{
    errmsg_ = "";
    rdr_->setComponent( data_.compidx_ );
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
	    if ( zistm_ )
		{ datasd.start *= 1000; datasd.step *= 1000; }
	    mPIEPAdj(Z,datasd.start,false); mPIEPAdj(Z,datasd.step,false);
	    if ( data_.isasc_ )
	    {
		*sd_.ostrm << toString(datasd.start) << '\t';
		*sd_.ostrm << toString(datasd.step) << '\t'
			   << data_.nrsamples_ << std::endl;
	    }
	    else
	    {
		mStrmBinWrite( datasd.start, float );
		mStrmBinWrite( datasd.step, float );
		mStrmBinWrite( data_.nrsamples_, int );
	    }
	}
    }

    if ( data_.remnull_ && trc_.isNull() )
	return Executor::MoreToDo();

    if ( data_.havenr_ )
    {
	int nr = trc_.info().nr;
	const float refnr = trc_.info().refnr;
	mPIEPAdj(TrcNr,nr,false);
	if ( data_.isasc_ )
	{
	    *sd_.ostrm << nr;
	    if ( data_.haverefnr_ )
		*sd_.ostrm << '\t' << toString(refnr);
	    if ( data_.havepos_ )
		*sd_.ostrm << '\t';
	}
	else
	{
	    mStrmBinWrite( nr, int );
	    if ( data_.haverefnr_ )
		mStrmBinWrite( refnr, float );
	}
    }

    if ( data_.havepos_ )
    {
	if ( data_.isxy_ )
	{
	    Coord coord = trc_.info().coord;
	    mPIEPAdj(Coord,coord,false);
	    if ( data_.isasc_ )
	    {
		*sd_.ostrm << toString(coord.x) << '\t';
		*sd_.ostrm << toString(coord.y);
	    }
	    else
	    {
		mStrmBinWrite( coord.x, double );
		mStrmBinWrite( coord.y, double );
	    }
	}
	else
	{
	    BinID bid = trc_.info().binid;
	    mPIEPAdj(BinID,bid,false);
	    if ( data_.isasc_ )
		*sd_.ostrm << bid.inl << '\t' << bid.crl;
	    else
	    {
		mStrmBinWrite( bid.inl, int );
		mStrmBinWrite( bid.crl, int );
	    }
	}
    }

    if ( Seis::isPS(data_.geom_) )
    {
	if ( data_.haveoffs_ )
	{
	    float offs = trc_.info().offset;
	    mPIEPAdj(Offset,offs,false);
	    if ( SI().xyInFeet() ) offs *= mToFeetFactor;
	    if ( data_.isasc_ )
		*sd_.ostrm << '\t' << offs;
	    else
		mStrmBinWrite( offs, float );
	}
	if ( data_.haveazim_ )
	{
	    const float azim = trc_.info().azimuth;
	    if ( data_.isasc_ )
		*sd_.ostrm << '\t' << azim;
	    else
		mStrmBinWrite( azim, float );
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
	{ errmsg_ = "Error during write"; return Executor::ErrorOccurred(); }

    nrdone_++;
    return Executor::MoreToDo();
}
