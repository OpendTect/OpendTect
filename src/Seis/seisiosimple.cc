/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/


#include "seisiosimple.h"

#include "envvars.h"
#include "genc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisimporter.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "trckeyzsampling.h"
#include "survinfo.h"
#include "survgeom.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "ascbinstream.h"
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
#include "uistrings.h"

#include <math.h>

#define mGetIBinStrm \
    ascbinistream binstrm( iStream(), !data_.isasc_ )
#define mGetOBinStrm \
    ascbinostream binstrm( oStream(), !data_.isasc_ )


SeisIOSimple::Data::Data( const char* filenm, Seis::GeomType gt )
    : subselpars_(*new IOPar("subsel"))
    , geom_(gt)
    , geomid_(mUdfGeomID)
{
    clear(true);
    if ( filenm && *filenm )
	fname_ = filenm;
}


SeisIOSimple::Data::Data( const SeisIOSimple::Data& oth )
    : subselpars_(*new IOPar("subsel"))
{
    *this = oth;
}


SeisIOSimple::Data& SeisIOSimple::Data::operator=( const SeisIOSimple::Data& d )
{
    if ( this == &d ) return *this;

    fname_ = d.fname_; seiskey_ = d.seiskey_;
    geom_ = d.geom_; geomid_ = d.geomid_;
    isasc_ = d.isasc_;
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
    linename_ = d.linename_;
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
    scaler_ = s ? s->clone() : nullptr;
}


void SeisIOSimple::Data::setResampler( SeisResampler* r )
{
    delete resampler_;
    resampler_ = r ? new SeisResampler( *r ) : nullptr;
}


void SeisIOSimple::Data::clear( bool survchg )
{
    deleteAndZeroPtr( resampler_ );
    nrdef_.start = 1; nrdef_.step = 1;
    if ( !survchg )
	return;

    subselpars_.setEmpty();
    fname_ = GetDataDir();
    seiskey_.setUdf();
    linename_.setEmpty();
    sd_.start = (float)SI().zRange(false).start;
    sd_.step = (float)SI().zRange(false).step;
    nrsamples_ = SI().zRange(false).nrSteps() + 1;
    inldef_.start = SI().sampling(false).hsamp_.start_.inl();
    crldef_.start = SI().sampling(false).hsamp_.start_.crl();
    inldef_.step = SI().inlStep();
    crldef_.step = SI().crlStep();
    nrcrlperinl_ = (SI().sampling(false).hsamp_.stop_.crl()
		 - SI().sampling(false).hsamp_.start_.crl())
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
    , trc_(*new SeisTrc)
    , isimp_(imp)
    , prevbid_(mUdf(int),0)
    , prevnr_(mUdf(int))
    , zistm_(SI().zIsTime())
{
    PtrMan<IOObj> ioobj = IOM().get( data_.seiskey_ );
    if ( !ioobj )
	return;

    const_cast<bool&>(zistm_) = ZDomain::isTime( ioobj->pars() );

    const Pos::GeomID geomid = data_.geomid_;
    SeisStoreAccess* sa;
    if ( isimp_ )
	sa = wrr_ = new SeisTrcWriter( *ioobj, geomid, &data_.geom_ );
    else
	sa = rdr_ = new SeisTrcReader( *ioobj, geomid, &data_.geom_ );

    errmsg_ = sa->errMsg();
    if ( !errmsg_.isEmpty() )
	return;

    PtrMan<Seis::SelData> seldata = Seis::SelData::get( data_.subselpars_ );
    if ( seldata && Survey::is2DGeom(geomid) )
	seldata->setGeomID( geomid );
    if ( seldata && !seldata->isAll() )
	sa->setSelData( seldata.release() );

    uiString errmsg;
    strm_ = od_stream::create( data_.fname_, isimp_, errmsg );
    if ( !strm_ )
    {
	errmsg_ = errmsg;
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
	errmsg_ = sios_.uiMessage();
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
	if ( zistm_ )
	    { data_.sd_.start *= .001; data_.sd_.step *= .001; }
    }

    trc_.info().sampling = data_.sd_;
    importer_ = new SeisImporter( new SeisIOSimpleImportReader(*this),
				  *wrr_, data_.geom_ );
}


uiString SeisIOSimple::uiMessage() const
{
    if ( importer_ )
	return importer_->uiMessage();

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


uiString SeisIOSimple::uiNrDoneText() const
{
    return importer_ ? importer_->uiNrDoneText() : tr("Traces written");
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

    return rv == 0 ? MoreToDo() : writeExpTrc();
}


int SeisIOSimple::readImpTrc( SeisTrc& trc )
{
    if ( !strm_->isOK() )
	return Finished();

    mGetIBinStrm;
    TrcKey tk;
    tk.setGeomID( data_.geomid_ );

    int nr = 1; float offs = 0, azim = 0, refnr = 0;
    Coord coord;
    const bool is2d = Seis::is2D(data_.geom_);
    const bool isps = Seis::isPS(data_.geom_);

    if ( data_.havenr_ )
    {
	binstrm.get( nr );
	if ( data_.haverefnr_ )
	    binstrm.get( refnr );
    }
    else
	nr = data_.nrdef_.start + nrdone_ * data_.nrdef_.step;

    if ( data_.havepos_ )
    {
	if ( data_.isxy_ )
	{
	    binstrm.get( coord.x ).get( coord.y );
	    ConstRefMan<Coords::CoordSystem> sicrs = SI().getCoordSystem();
	    ConstRefMan<Coords::CoordSystem> datacrs = data_.getCoordSys();
	    if ( sicrs && datacrs && !(*sicrs == *datacrs) )
		coord = sicrs->convertFrom( coord, *datacrs );

	    if ( is2d )
	    {
		if ( data_.havenr_ )
		    tk.setTrcNr( nr );
		else
		{
		    const Survey::Geometry* geom =
				Survey::GM().getGeometry( data_.geomid_ );
		    if ( geom && geom->as2D() )
			tk = geom->nearestTrace( coord );
		    else
			tk = Survey::GM().nearestTrace( coord, true );
		}
	    }
	    else
		tk.setPosition( SI().transform(coord) );
	}
	else
	{
	    int linenr, trcnr;
	    binstrm.get( linenr ).get( trcnr );
	    if ( is2d )
		tk.setTrcNr( trcnr ); //Should not be hit, always with X/Y
	    else
		tk.setPosition( BinID(linenr,trcnr) );
	}
    }
    else
    {
	const int nrposdone = isps ? nrdone_ / data_.nroffsperpos_ : nrdone_;
	if ( is2d )
	{
	    coord.x = data_.startpos_.x + nrposdone * data_.steppos_.x;
	    coord.y = data_.startpos_.y + nrposdone * data_.steppos_.y;
	    const Survey::Geometry* geom =
				Survey::GM().getGeometry( data_.geomid_ );
	    if ( geom && geom->as2D() )
		tk = geom->nearestTrace( coord );
	    else
		tk = Survey::GM().nearestTrace( coord, true );
	}
	else
	{
	    const int nrinl = nrposdone / data_.nrcrlperinl_;
	    const int nrcrl = nrposdone % data_.nrcrlperinl_;
	    const int inlnr = data_.inldef_.start + nrinl * data_.inldef_.step;
	    const int crlnr = data_.crldef_.start + nrcrl * data_.crldef_.step;
	    tk.setPosition( BinID(inlnr,crlnr) );
	}
    }

    const TrcKey& csttk = const_cast<const TrcKey&>( tk );
    if ( isps )
    {
	if ( !data_.haveoffs_ )
	    offs = data_.offsdef_.start + offsnr_ * data_.offsdef_.step;
	else
	    binstrm.get( offs );
	if ( data_.haveazim_ )
	    binstrm.get( azim );
	if (  (is2d && csttk.trcNr() != prevnr_) ||
	     (!is2d && csttk.position() != prevbid_) )
	    offsnr_ = 0;
	else
	    offsnr_++;
    }

    trc.info() = trc_.info();
    if ( !strm_->isOK() || !trc.reSize(data_.nrsamples_,0) )
	return Finished();

    if ( is2d )
    {
	int trcnr = csttk.trcNr();
	mPIEPAdj(TrcNr,trcnr,true);
	tk.setTrcNr( trcnr );
    }
    else
    {
	BinID bid = csttk.position();
	mPIEPAdj(BinID,bid,true);
	tk.setPosition( bid );
    }

    trc.info().setTrcKey( csttk );
    trc.info().calcCoord();
    if ( trc.info().coord.isUdf() && !coord.isUdf() )
	trc.info().coord = coord;

    mPIEPAdj(Coord,trc.info().coord,true);
    mPIEPAdj(Offset,offs,true);
    trc.info().offset = offs;
    trc.info().azimuth = azim;
    trc.info().seqnr_ = nrDone();
    trc.info().refnr = refnr;

    prevbid_ = csttk.position();
    prevnr_ = csttk.trcNr();
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
	    ptr = getNextWord( ptr, buf );
	    float val = 0; Conv::set( val, bufptr );
	    mApplyScalerAndSetTrcVal;
	}
    }

    return strm_->isOK() ? MoreToDo() : Finished();
}


int SeisIOSimple::readExpTrc()
{
    errmsg_ = uiString::emptyString();
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
    mGetOBinStrm;

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
	const float refnr = trc_.info().refnr;
	mPIEPAdj(TrcNr,nr,false);
	binstrm.add( nr );
	if ( data_.haverefnr_ )
	    binstrm.add( refnr );
    }

    if ( data_.havepos_ )
    {
	if ( data_.isxy_ )
	{
	    Coord coord = trc_.info().coord;
	    mPIEPAdj(Coord,coord,false);
	    ConstRefMan<Coords::CoordSystem> sicrs = SI().getCoordSystem();
	    ConstRefMan<Coords::CoordSystem> datacrs = data_.getCoordSys();
	    if ( sicrs && datacrs && !(*sicrs == *datacrs) )
		coord = datacrs->convertFrom( coord, *sicrs );

	    binstrm.add( coord.x ).add( coord.y );
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
	    float offs = trc_.info().offset;
	    mPIEPAdj(Offset,offs,false);
	    binstrm.add( offs );
	}
	if ( data_.haveazim_ )
	    binstrm.add( trc_.info().azimuth );
    }

    float val;
    static const bool isswapped = GetEnvVarYN("OD_SIMPLE_FILE_BINARY_SWAPPED");
    for ( int idx=0; idx<data_.nrsamples_; idx++ )
    {
	val = trc_.get( idx, 0 );
	if ( data_.scaler_ ) val = (float) data_.scaler_->scale( val );
	if ( isswapped ) SwapBytes( &val, sizeof(float) );
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
