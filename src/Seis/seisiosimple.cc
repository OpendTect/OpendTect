/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2003
-*/


#include "seisiosimple.h"

#include "envvars.h"
#include "genc.h"
#include "seisprovider.h"
#include "seisstorer.h"
#include "seisimporter.h"
#include "seistrc.h"
#include "seisrangeseldata.h"
#include "seisresampler.h"
#include "survinfo.h"
#include "survgeom.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "ascbinstream.h"
#include "ptrman.h"
#include "ioobj.h"
#include "iopar.h"
#include "scaler.h"
#include "posimpexppars.h"
#include "zdomain.h"
#include "msgh.h"
#include "uistrings.h"

#include <math.h>

#define mGetIBinStrm \
    ascbinistream binstrm( iStream(), !data_.isasc_ )
#define mGetOBinStrm \
    ascbinostream binstrm( oStream(), !data_.isasc_ )


SeisIOSimple::Data::Data( const char* filenm, Seis::GeomType gt )
	: scaler_(0)
	, resampler_(0)
	, subselpars_(*new IOPar("subsel"))
	, geom_(gt)
	, geomid_(mUdfGeomID)
	, compidx_(0)
	, coordsys_(0)
{
    clear(true);
    isasc_ = havepos_ = havesd_ = true;
    havenr_ = haverefnr_ = haveoffs_ = haveazim_
	    = isxy_ = remnull_ = sdzdomscaled_ = false;

    if ( filenm && *filenm )
	fname_ = filenm;
}


SeisIOSimple::Data::Data( const SeisIOSimple::Data& d )
	: scaler_(0)
	, resampler_(0)
	, subselpars_(*new IOPar("subsel"))
	, coordsys_(0)
{
    *this = d;
}


SeisIOSimple::Data& SeisIOSimple::Data::operator=( const SeisIOSimple::Data& d )
{
    if ( this == &d ) return *this;

    fname_ = d.fname_; seiskey_ = d.seiskey_;
    geom_ = d.geom_; isasc_ = d.isasc_;
    havesd_ = d.havesd_; sd_ = d.sd_; sdzdomscaled_ = d.sdzdomscaled_;
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
    geomid_ = d.geomid_;
    subselpars_ = d.subselpars_;
    compidx_ = d.compidx_;
    coordsys_ = d.coordsys_;

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

    subselpars_.setEmpty();
    fname_ = GetDataDir(); seiskey_.setInvalid(); geomid_ = Pos::GeomID();
    const auto inlrg = SI().inlRange();
    const auto crlrg = SI().crlRange();
    sd_.start = (float)SI().zRange().start;
    sd_.step = (float)SI().zRange().step;
    nrsamples_ = SI().zRange().nrSteps() + 1;
    inldef_.start = inlrg.start; inldef_.step = inlrg.step;
    crldef_.start = crlrg.start; crldef_.step = crlrg.step;
    nrcrlperinl_ = crlrg.nrSteps() + 1;
    nroffsperpos_ = 10; // cannot think of a good default ...
    startpos_ = SI().transform( BinID(inldef_.start,crldef_.start) );
    Coord nextpos = SI().transform( BinID(inldef_.start+inldef_.step,
					  crldef_.start+crldef_.step) );
    steppos_.x_ = fabs( nextpos.x_ - startpos_.x_ );
    steppos_.y_ = fabs( nextpos.y_ - startpos_.y_ );
    offsdef_.start = 0; offsdef_.step = SI().crlDistance();
    compidx_ = 0;
    coordsys_ = 0;
}


SeisIOSimple::SeisIOSimple( const Data& d, bool imp )
	: Executor( imp ? "Import Seismics from simple file"
			: "Export Seismics to simple file" )
	, data_(d)
	, trc_(*new SeisTrc)
	, isimp_(imp)
	, prevbid_(mUdf(int),0)
	, prevnr_(mUdf(int))
	, zistm_(SI().zIsTime())
{
    PtrMan<IOObj> ioobj = data_.seiskey_.getIOObj();
    if ( !ioobj )
	return;
    const_cast<bool&>(zistm_) = ZDomain::isTime( ioobj->pars() );

    if ( isimp_ )
    {
	storer_ = new Storer( *ioobj );
	if ( !storer_->isUsable() )
	    return;
	if ( storer_->is2D() && data_.geomid_.isValid() )
	    storer_->setFixedGeomID( data_.geomid_ );
    }
    else
    {
	uiRetVal uirv;
	prov_ = Provider::create( d.seiskey_, &uirv );
	if ( !prov_ )
	    { errmsg_ = uirv; return; }

	const bool havegeomid2d = data_.geomid_.isValid()
				&& data_.geomid_.is2D();
	auto* seldata = Seis::SelData::get( data_.subselpars_ );
	if ( !seldata && havegeomid2d )
	    seldata = new Seis::RangeSelData( data_.geomid_ );
	if ( seldata && !havegeomid2d && seldata->isAll() )
	    deleteAndZeroPtr( seldata );
	if ( seldata && seldata->isRange() && havegeomid2d )
	    seldata->asRange()->setGeomID( data_.geomid_ );

	prov_->setSelData( seldata );
    }

    strm_ = od_stream::create( data_.fname_, isimp_, errmsg_ );
    if ( !strm_ )
	return;

    if ( isimp_ )
	startImpRead();
}


SeisIOSimple::~SeisIOSimple()
{
    delete importer_;
    delete prov_;
    delete storer_;
    delete strm_;
    delete &trc_;
}


od_istream& SeisIOSimple::iStream()
{
    return *static_cast<od_istream*>( strm_ );
}


od_ostream& SeisIOSimple::oStream()
{
    return *static_cast<od_ostream*>( strm_ );
}


class SeisIOSimpleImportReader : public SeisImporter::Reader
{
public:

SeisIOSimpleImportReader( SeisIOSimple& sios )
	: sios_(sios)		{}

const char* name() const	{ return "Simple File"; }
const char* implName() const	{ return sios_.iStream().fileName(); }
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
    mGetIBinStrm;
    if ( data_.havesd_ )
    {
	binstrm.get( data_.sd_.start ).get( data_.sd_.step )
	       .get( data_.nrsamples_ );
	if ( !strm_->isOK() )
	    { errmsg_ = tr("Input file contains no data"); return; }
	if ( data_.sdzdomscaled_ && zistm_ )
	    { data_.sd_.start *= .001; data_.sd_.step *= .001; }
    }

    trc_.info().sampling_ = data_.sd_;
    importer_ = new SeisImporter( new SeisIOSimpleImportReader(*this),
				  *storer_, data_.geom_ );
}


uiString SeisIOSimple::message() const
{
    if ( importer_ )
	return importer_->message();

    return errmsg_.isEmpty() ? tr("Handling traces") : errmsg_;
}


od_int64 SeisIOSimple::nrDone() const
{
    return importer_ ? importer_->nrDone() : nrdone_;
}


od_int64 SeisIOSimple::totalNr() const
{
    return importer_ ? importer_->totalNr() : -1;
}


uiString SeisIOSimple::nrDoneText() const
{
    return importer_ ? importer_->nrDoneText() : tr("Traces written");
}


int SeisIOSimple::nextStep()
{
    if ( !errmsg_.isEmpty() )
	return -1;
    else if ( isimp_ )
    {
	int rv = importer_ ? importer_->nextStep() : -1;
	if ( rv == Finished() && importer_->nrSkipped() > 0 )
	    UsrMsg( BufferString("Warning: ",importer_->nrSkipped(),
				 " traces were rejected during import") );
	return rv;
    }

    int rv = readExpTrc();
    if ( rv < 0 )
	return errmsg_.isEmpty() ? Finished() : ErrorOccurred();

    return rv == Finished() ? Finished() : writeExpTrc();
}


int SeisIOSimple::readImpTrc( SeisTrc& trc )
{
    if ( !strm_->isOK() )
	return Finished();

    mGetIBinStrm;
    BinID bid; Coord coord; int nr = 1; float offs = 0, azim = 0, refnr = 0;
    const bool is2d = Seis::is2D(data_.geom_);
    const bool isps = Seis::isPS(data_.geom_);

    if ( !data_.havenr_ )
	nr = data_.nrdef_.start + nrdone_ * data_.nrdef_.step;
    else
    {
	binstrm.get( nr );
	if ( data_.haverefnr_ )
	    binstrm.get( refnr );
    }

    if ( !data_.havepos_ )
    {
	const int nrposdone = isps ? nrdone_ / data_.nroffsperpos_ : nrdone_;
	if ( is2d )
	{
	    coord.x_ = data_.startpos_.x_ + nrposdone * data_.steppos_.x_;
	    coord.y_ = data_.startpos_.y_ + nrposdone * data_.steppos_.y_;
	    bid.crl() = nr;
	}
	else
	{
	    const int nrinl = nrposdone / data_.nrcrlperinl_;
	    const int nrcrl = nrposdone % data_.nrcrlperinl_;
	    bid.inl() = data_.inldef_.start + nrinl * data_.inldef_.step;
	    bid.crl() = data_.crldef_.start + nrcrl * data_.crldef_.step;
	    coord = SI().transform( bid );
	}
    }
    else
    {
	if ( data_.isxy_ )
	{
	    binstrm.get( coord.x_ ).get( coord.y_ );
	    bid = SI().transform( coord );
	}
	else
	{
	    binstrm.get( bid.inl() ).get( bid.crl() );
	    coord = SI().transform( bid );
	}
    }

    if ( isps )
    {
	if ( !data_.haveoffs_ )
	    offs = data_.offsdef_.start + offsnr_ * data_.offsdef_.step;
	else
	    binstrm.get( offs );
	if ( data_.haveazim_ )
	    binstrm.get( azim );
	if ( (is2d && nr != prevnr_) || (!is2d && bid != prevbid_) )
	    offsnr_ = 0;
	else
	    offsnr_++;
    }

    trc.info() = trc_.info();
    if ( !strm_->isOK() || !trc.reSize(data_.nrsamples_,0) )
	return Finished();

    mPIEPAdj(BinID,bid,true); mPIEPAdj(Coord,coord,true);
    mPIEPAdj(TrcNr,nr,true); mPIEPAdj(Offset,offs,true);

    if ( is2d )
	trc.info().setPos( Bin2D(data_.geomid_,nr) );
    else
	trc.info().setPos( bid );
    prevbid_ = bid;
    trc.info().coord_ = coord;
    trc.info().offset_ = SI().xyInFeet() ? offs * mFromFeetFactorF : offs;
    trc.info().azimuth_ = azim;
    trc.info().refnr_ = refnr;
    prevnr_ = nr;
#   define mApplyScalerAndSetTrcVal \
	if ( data_.scaler_ ) \
	    val = (float) data_.scaler_->scale( val ); \
        if ( isswapped ) SwapBytes( &val, sizeof(float) ); \
	trc.set( idx, val, 0 )

    static const bool isswapped = GetEnvVarYN("OD_SIMPLE_FILE_BINARY_SWAPPED");
    if ( !data_.isasc_ )
    {
	for ( int idx=0; idx<data_.nrsamples_; idx++ )
	{
	    float val = 0; binstrm.get( val );
	    if ( strm_->isBad() )
		return Finished();
	    mApplyScalerAndSetTrcVal;
	}
    }
    else
    {
	    // got support calls because of bad files too often, so taking
	    // evasive action here:
	BufferString line; iStream().getLine( line );
	if ( strm_->isBad() )
	    return Finished();
	const char* ptr = line.buf();
	char buf[1024]; const char* bufptr = buf;
	for ( int idx=0; idx<data_.nrsamples_; idx++ )
	{
	    ptr = getNextNonBlanks( ptr, buf );
	    float val = 0; Conv::set( val, bufptr );
	    mApplyScalerAndSetTrcVal;
	}
    }

    return strm_->isOK() ? MoreToDo() : Finished();
}


int SeisIOSimple::readExpTrc()
{
    if ( !prov_ )
	return ErrorOccurred();

    errmsg_.setEmpty();
    prov_->selectComponent( data_.compidx_ );
    const uiRetVal uirv = prov_->getNext( trc_ );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Finished();

	errmsg_ = uirv;
	return ErrorOccurred();
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
    mGetOBinStrm;

    if ( firsttrc_ )
    {
	firsttrc_ = false;
	data_.nrsamples_ = trc_.size();
	if ( data_.havesd_ )
	{
	    SamplingData<float> datasd = trc_.info().sampling_;
	    mPIEPAdj(Z,datasd.start,false); mPIEPAdj(Z,datasd.step,false);
	    if ( data_.sdzdomscaled_ && zistm_ )
		{ datasd.start *= 1000; datasd.step *= 1000; }
	    binstrm.add( datasd.start )
		   .add( datasd.step )
		   .add( data_.nrsamples_, od_newline );
	}
    }

    if ( data_.remnull_ && trc_.isNull() )
	return MoreToDo();

    if ( data_.havenr_ )
    {
	int nr = trc_.info().trcNr();
	const float refnr = trc_.info().refnr_;
	mPIEPAdj(TrcNr,nr,false);
	binstrm.add( nr );
	if ( data_.haverefnr_ )
	    binstrm.add( refnr );
    }

    if ( data_.havepos_ )
    {
	if ( data_.isxy_ )
	{
	    Coord coord = trc_.info().coord_;
	    mPIEPAdj(Coord,coord,false);
	    coord = data_.getCoordSys()->convertFrom(
						coord,*SI().getCoordSystem() );
	    binstrm.add( coord.x_ ).add( coord.y_ );
	}
	else
	{
	    BinID bid = trc_.info().binID();
	    mPIEPAdj(BinID,bid,false);
	    binstrm.add( bid.inl() ).add( bid.crl() );
	}
    }

    if ( Seis::isPS(data_.geom_) )
    {
	if ( data_.haveoffs_ )
	{
	    float offs = trc_.info().offset_;
	    mPIEPAdj(Offset,offs,false);
	    if ( SI().xyInFeet() ) offs *= mToFeetFactorF;
	    binstrm.add( offs );
	}
	if ( data_.haveazim_ )
	    binstrm.add( trc_.info().azimuth_ );
    }

    float val;
    static const bool isswapped = GetEnvVarYN("OD_SIMPLE_FILE_BINARY_SWAPPED");
    for ( int idx=0; idx<data_.nrsamples_; idx++ )
    {
	val = trc_.get( idx, 0 );
	if ( data_.scaler_ ) val = (float) data_.scaler_->scale( val );
	if ( isswapped )
	    SwapBytes( &val, sizeof(float) );
	binstrm.add( val, idx == data_.nrsamples_-1 ? od_newline : od_tab );
    }

    if ( !strm_->isOK() )
    {
	errmsg_ = uiStrings::phrCannotWrite( toUiString(strm_->fileName()) );
	strm_->addErrMsgTo( errmsg_ );
	return ErrorOccurred();
    }

    nrdone_++;
    return MoreToDo();
}
