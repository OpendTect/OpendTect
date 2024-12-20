/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisiosimple.h"

#include "ascbinstream.h"
#include "envvars.h"
#include "genc.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "msgh.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "posimpexppars.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisimporter.h"
#include "seisread.h"
#include "seisresampler.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "zdomain.h"

#include <math.h>

#define mGetIBinStrm \
    ascbinistream binstrm( iStream(), !data_.isasc_ )
#define mGetOBinStrm \
    ascbinostream binstrm( oStream(), !data_.isasc_ )


SeisIOSimple::Data::Data( const char* filenm, Seis::GeomType gt )
    : geom_(gt)
    , subselpars_(*new IOPar("subsel"))
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
    deleteAndNullPtr( resampler_ );
    nrdef_.start_ = 1; nrdef_.step_ = 1;
    if ( !survchg )
	return;

    subselpars_.setEmpty();
    fname_ = GetDataDir();
    seiskey_.setUdf();
    linename_.setEmpty();
    sd_.start_ = (float)SI().zRange(false).start_;
    sd_.step_ = (float)SI().zRange(false).step_;
    nrsamples_ = SI().zRange(false).nrSteps() + 1;
    inldef_.start_ = SI().sampling(false).hsamp_.start_.inl();
    crldef_.start_ = SI().sampling(false).hsamp_.start_.crl();
    inldef_.step_ = SI().inlStep();
    crldef_.step_ = SI().crlStep();
    nrcrlperinl_ = (SI().sampling(false).hsamp_.stop_.crl()
		 - SI().sampling(false).hsamp_.start_.crl())
                   / crldef_.step_ + 1;
    nroffsperpos_ = 10; // cannot think of a good default ...
    startpos_ = SI().transform( BinID(inldef_.start_,crldef_.start_) );
    Coord nextpos = SI().transform( BinID(inldef_.start_+inldef_.step_,
                                          crldef_.start_+crldef_.step_) );
    steppos_.x_ = fabs( nextpos.x_ - startpos_.x_ );
    steppos_.y_ = fabs( nextpos.y_ - startpos_.y_ );
    offsdef_.start_ = 0; offsdef_.step_ = SI().crlDistance();
    compidx_ = 0;
}


SeisIOSimple::SeisIOSimple( const Data& d, bool imp )
    : Executor( imp ? "Import Seismic from simple file"
		    : "Export Seismic to simple file" )
    , data_(d)
    , trc_(*new SeisTrc)
    , isimp_(imp)
    , prevbid_(mUdf(int),0)
    , prevnr_(mUdf(int))
    , zistm_(SI().zIsTime())
{
    const bool is2d = Seis::is2D( data_.geom_ );
    if ( is2d )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(data_.geomid_));
	if ( geom2d && !geom2d->isEmpty() )
	    geom2d_ = geom2d;
    }

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

const char* name() const override	{ return "Simple File"; }
const char* implName() const override	{ return sios_.iStream().fileName(); }
bool fetch( SeisTrc& trc ) override
{
    int rv = sios_.readImpTrc( trc );
    if ( rv > 0 )
    {
	sios_.nrdone_++;
	return true;
    }

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
        binstrm.get( data_.sd_.start_ ).get( data_.sd_.step_ )
	       .get( data_.nrsamples_ );
	if ( !strm_->isOK() )
	{ errmsg_ = tr("Input file contains no data"); return; }
	if ( zistm_ )
        { data_.sd_.start_ *= .001; data_.sd_.step_ *= .001; }
    }

    trc_.info().sampling_ = data_.sd_;
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
    return importer_ ? importer_->totalNr() :
			(rdr_ ? rdr_->expectedNrTraces() : 0);
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
        nr = data_.nrdef_.start_ + nrdone_ * data_.nrdef_.step_;

    if ( is2d )
	tk.setTrcNr( nr );

    if ( data_.havepos_ )
    {
	if ( data_.isxy_ || is2d )
	{
            binstrm.get( coord.x_ ).get( coord.y_ );
	    ConstRefMan<Coords::CoordSystem> sicrs = SI().getCoordSystem();
	    ConstRefMan<Coords::CoordSystem> datacrs = data_.getCoordSys();
	    if ( sicrs && datacrs && !(*sicrs == *datacrs) )
		coord = sicrs->convertFrom( coord, *datacrs );

	    if ( !is2d )
		tk.setPosition( SI().transform(coord) );
	}
	else
	{
	    int linenr, trcnr;
	    binstrm.get( linenr ).get( trcnr );
	    if ( !is2d )
		tk.setPosition( BinID(linenr,trcnr) );
	}
    }
    else
    {
	const int nrposdone = isps ? nrdone_ / data_.nroffsperpos_ : nrdone_;
	if ( is2d )
	{
            coord.x_ = data_.startpos_.x_ + nrposdone * data_.steppos_.x_;
            coord.y_ = data_.startpos_.y_ + nrposdone * data_.steppos_.y_;
	}
	else
	{
	    const int nrinl = nrposdone / data_.nrcrlperinl_;
	    const int nrcrl = nrposdone % data_.nrcrlperinl_;
	    const int inlnr = data_.inldef_.start_ + nrinl*data_.inldef_.step_;
	    const int crlnr = data_.crldef_.start_ + nrcrl*data_.crldef_.step_;
	    tk.setPosition( BinID(inlnr,crlnr) );
	}
    }

    const TrcKey& csttk = const_cast<const TrcKey&>( tk );
    if ( isps )
    {
	if ( !data_.haveoffs_ )
            offs = data_.offsdef_.start_ + offsnr_ * data_.offsdef_.step_;
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

	// if line exists, use coord from existing geometry
	if ( geom2d_ )
	    coord = geom2d_->toCoord( nr );
    }
    else
    {
	BinID bid = csttk.position();
	mPIEPAdj(BinID,bid,true);
	tk.setPosition( bid );
    }

    trc.info().setTrcKey( csttk );
    if ( !is2d )
	trc.info().calcCoord();

    if ( is2d || (trc.info().coord_.isUdf() && !coord.isUdf()) )
	trc.info().coord_ = coord;

    mPIEPAdj(Coord,trc.info().coord_,true);
    mPIEPAdj(Offset,offs,true);
    trc.info().offset_ = offs;
    trc.info().azimuth_ = azim;
    trc.info().seqnr_ = nrDone();
    trc.info().refnr_ = refnr;

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
	    SamplingData<float> datasd = trc_.info().sampling_;
	    if ( zistm_ )
            { datasd.start_ *= 1000; datasd.step_ *= 1000; }
            mPIEPAdj(Z,datasd.start_,false); mPIEPAdj(Z,datasd.step_,false);
            binstrm.add( datasd.start_ )
                    .add( datasd.step_ )
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
	    ConstRefMan<Coords::CoordSystem> sicrs = SI().getCoordSystem();
	    ConstRefMan<Coords::CoordSystem> datacrs = data_.getCoordSys();
	    if ( sicrs && datacrs && !(*sicrs == *datacrs) )
		coord = datacrs->convertFrom( coord, *sicrs );

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
