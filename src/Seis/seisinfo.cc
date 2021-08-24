/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/


#include "seisinfo.h"
#include "seiscommon.h"
#include "seispacketinfo.h"
#include "seisbounds.h"
#include "seistrc.h"
#include "posauxinfo.h"
#include "survinfo.h"
#include "strmprov.h"
#include "file.h"
#include "iopar.h"
#include "trckeyzsampling.h"
#include "enums.h"
#include "envvars.h"
#include "keystrs.h"
#include "timeser.h"
#include "ctxtioobj.h"
#include "od_istream.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seis2dlineio.h"
#include "uistrings.h"

#include <float.h>
#include <iostream>
#include <math.h>

const char* SeisTrcInfo::sSamplingInfo = "Sampling information";
const char* SeisTrcInfo::sNrSamples = "Nr of samples";
const char* SeisPacketInfo::sBinIDs = "BinID range";
const char* SeisPacketInfo::sZRange = "Z range";


static BufferString getUsrInfo()
{
    BufferString ret;
    const char* envstr = GetEnvVar( "DTECT_SEIS_USRINFO" );
    if ( !envstr || !File::exists(envstr) )
	return ret;

    od_istream strm( envstr );
    BufferString linebuf;
    while ( strm.isOK() )
    {
	strm.getLine( linebuf );
	if ( !linebuf.isEmpty() )
	    ret.addNewLine().add( linebuf );
    }

    return ret;
}


BufferString SeisPacketInfo::defaultusrinfo;


void SeisPacketInfo::initClass()
{ defaultusrinfo = getUsrInfo(); }

class SeisEnum
{
public:
    typedef Seis::SelType SelType;
	    mDeclareEnumUtils(SelType)
    typedef Seis::GeomType GeomType;
	    mDeclareEnumUtils(GeomType)
    typedef Seis::WaveType WaveType;
	    mDeclareEnumUtils(WaveType)
    typedef Seis::DataType DataType;
	    mDeclareEnumUtils(DataType)
};

mDefineEnumUtils(SeisEnum,SelType,"Selection type")
{
	sKey::Range(),
	sKey::Table(),
	sKey::Polygon(),
	0
};

mDefineEnumUtils(SeisEnum,GeomType,"Geometry type")
{
	"3D Volume",
	"Pre-Stack Volume",
	"2D Line",
	"Line 2D Pre-Stack",
	0
};

mDefineEnumUtils(SeisEnum,WaveType,"Wave type")
{
	"P",
	"Sh",
	"Sv",
	"Other",
	0
};

mDefineEnumUtils(SeisEnum,DataType,"Data type")
{
	"Amplitude",
	"Dip",
	"Frequency",
	"Phase",
	"AVO Gradient",
	"Azimuth",
	"Classification",
	"Other",
	0
};

const char* Seis::nameOf( Seis::SelType st )
{ return SeisEnum::getSelTypeString(st); }

const char* Seis::nameOf( Seis::GeomType gt )
{ return SeisEnum::getGeomTypeString(gt); }

const char* Seis::nameOf( Seis::DataType dt )
{ return SeisEnum::getDataTypeString(dt); }

const char* Seis::nameOf( Seis::WaveType wt )
{ return SeisEnum::getWaveTypeString(wt); }

Seis::SelType Seis::selTypeOf( const char* s )
{ SeisEnum::SelType res; SeisEnum::parseEnumSelType(s,res); return res; }

Seis::GeomType Seis::geomTypeOf( const char* s )
{ SeisEnum::GeomType res; SeisEnum::parseEnumGeomType(s,res); return res; }

Seis::DataType Seis::dataTypeOf( const char* s )
{
    if ( sKey::Steering() == s )
	return Seis::Dip;
    SeisEnum::DataType res; SeisEnum::parseEnumDataType(s,res);
    return res;
}

Seis::WaveType Seis::waveTypeOf( const char* s )
{ SeisEnum::WaveType res; SeisEnum::parseEnumWaveType(s,res); return res; }

const char** Seis::selTypeNames()
{ return SeisEnum::SelTypeNames(); }

const char** Seis::geomTypeNames()
{ return SeisEnum::GeomTypeNames(); }

const char** Seis::dataTypeNames()
{ return SeisEnum::DataTypeNames(); }

const char** Seis::waveTypeNames()
{ return SeisEnum::WaveTypeNames(); }

bool Seis::isAngle( Seis::DataType dt )
{ return dt==Seis::Phase || dt==Seis::Azimuth; }

void Seis::putInPar( Seis::GeomType gt, IOPar& iop )
{
    iop.set( sKey::Geometry(), Seis::nameOf(gt) );
}

bool Seis::getFromPar( const IOPar& iop, Seis::GeomType& gt )
{
    const char* res = iop.find( sKey::Geometry() );
    if ( !res || !*res ) return false;
    gt = geomTypeOf( res );
    return true;
}


bool Seis::is2DGeom( const IOPar& iop )
{
    GeomType gt = Vol;
    getFromPar( iop, gt );
    return is2D( gt );
}

uiString Seis::dataName( GeomType tp, bool explprepost )
{
    const bool both_2d_3d = SI().survDataType()==SurveyInfo::Both2DAnd3D;
    return uiStrings::sVolDataName( is2D(tp), is3D(tp), isPS(tp),
				    both_2d_3d, explprepost );
}

bool Seis::isPSGeom( const IOPar& iop )
{
    GeomType gt = Vol;
    getFromPar( iop, gt );
    return isPS( gt );
}


mDefineEnumUtils(SeisTrcInfo,Fld,"Header field") {
	"Trace number",
	"Pick position",
	"Ref/SP number",
	"X-coordinate",
	"Y-coordinate",
	"In-line",
	"Cross-line",
	"Offset",
	"Azimuth",
	0
};


void SeisPacketInfo::clear()
{
    usrinfo = defaultusrinfo;
    nr = 0;
    fullyrectandreg = false;
    cubedata = 0;
    SI().sampling(false).hsamp_.get( inlrg, crlrg );
    zrg = SI().zRange(false);
    inlrev = crlrev = false;
}


float SeisTrcInfo::defaultSampleInterval( bool forcetime )
{
    float defsr = SI().zStep();
    if ( SI().zIsTime() || !forcetime )
	return defsr;

    defsr /= SI().zInFeet() ? 5000 : 2000; // div by velocity
    int ival = (int)(defsr * 1000 + .5);
    return ival * 0.001f;
}


double SeisTrcInfo::getValue( SeisTrcInfo::Fld fld ) const
{
    switch ( fld )
    {
    case CoordX:	return coord.x;
    case CoordY:	return coord.y;
    case BinIDInl:	return inl();
    case BinIDCrl:	return crl();
    case Offset:	return offset;
    case Azimuth:	return azimuth;
    case RefNr:		return refnr;
    case Pick:		return pick;
    default:		return nr;
    }
}


void SeisTrcInfo::getAxisCandidates( Seis::GeomType gt,
				     TypeSet<SeisTrcInfo::Fld>& flds )
{
    flds.erase();

    if ( Seis::is2D(gt) )
	{ flds += TrcNr; flds += RefNr; }
    else
	{ flds += BinIDInl; flds += BinIDCrl; }
    if ( Seis::isPS(gt) )
	{ flds += Offset; flds += Azimuth; }

    // Coordinates are always an option
    flds += CoordX; flds += CoordY;
}


int SeisTrcInfo::getDefaultAxisFld( Seis::GeomType gt,
				    const SeisTrcInfo* ti ) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    if ( !ti )
	return isps ? Offset : (is2d ? TrcNr : BinIDCrl);

    if ( isps && !Seis::equalOffset(ti->offset,offset) )
	return Offset;
    if ( is2d && ti->nr != nr )
	return TrcNr;
    if ( !is2d && ti->crl() != crl() )
	return BinIDCrl;
    if ( !is2d && ti->inl() != inl() )
	return BinIDInl;

    // 'normal' doesn't apply, try coordinates
    return mIsZero(ti->coord.x-coord.x,.1) ? CoordY : CoordX;
}


#define mIOIOPar(fn,fld,memb) iopar.fn( FldNames()[(int)fld], memb )

void SeisTrcInfo::getInterestingFlds( Seis::GeomType gt, IOPar& iopar ) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );

    if ( isps )
    {
	mIOIOPar( set, Offset, offset );
	mIOIOPar( set, Azimuth, azimuth );
    }

    if ( is2d )
    {
	mIOIOPar( set, TrcNr, nr );
	if ( refnr && !mIsUdf(refnr) )
	    mIOIOPar( set, RefNr, refnr );
    }
    else
    {
	mIOIOPar( set, BinIDInl, inl() );
	mIOIOPar( set, BinIDCrl, crl() );
	iopar.set( sKey::Position(), binID().toString() );
    }

    mIOIOPar( set, CoordX, coord.x );
    mIOIOPar( set, CoordY, coord.y );

    if ( pick && !mIsUdf(pick) )
	mIOIOPar( set, Pick, pick );
    if ( refnr && !mIsUdf(refnr) )
	mIOIOPar( set, RefNr, refnr );
}


void SeisTrcInfo::setPSFlds( const Coord& rcv, const Coord& src, bool setpos )
{
    offset = (float) rcv.distTo( src );
    azimuth = mCast(float, Math::Atan2( rcv.y - src.y, rcv.x - src.x ) );
    if ( setpos )
    {
	coord.x = .5 * (rcv.x + src.x);
	coord.y = .5 * (rcv.y + src.y);
	setBinID( SI().transform(coord) );
    }
}


void SeisTrcInfo::usePar( const IOPar& iopar )
{
    mIOIOPar( get, TrcNr,	nr );
    BinID bid( binID() );
    mIOIOPar( get, BinIDInl,	bid.inl() );
    mIOIOPar( get, BinIDCrl,	bid.crl() );
    setBinID( bid );
    mIOIOPar( get, CoordX,	coord.x );
    mIOIOPar( get, CoordY,	coord.y );
    mIOIOPar( get, Offset,	offset );
    mIOIOPar( get, Azimuth,	azimuth );
    mIOIOPar( get, Pick,	pick );
    mIOIOPar( get, RefNr,	refnr );

    iopar.get( sSamplingInfo, sampling.start, sampling.step );
}


void SeisTrcInfo::fillPar( IOPar& iopar ) const
{
    mIOIOPar( set, TrcNr,	nr );
    mIOIOPar( set, BinIDInl,	inl() );
    mIOIOPar( set, BinIDCrl,	crl() );
    mIOIOPar( set, CoordX,	coord.x );
    mIOIOPar( set, CoordY,	coord.y );
    mIOIOPar( set, Offset,	offset );
    mIOIOPar( set, Azimuth,	azimuth );
    mIOIOPar( set, Pick,	pick );
    mIOIOPar( set, RefNr,	refnr );

    iopar.set( sSamplingInfo, sampling.start, sampling.step );
}


bool SeisTrcInfo::dataPresent( float t, int trcsz ) const
{
    return t > sampling.start-1e-6 && t < samplePos(trcsz-1) + 1e-6;
}


int SeisTrcInfo::nearestSample( float t ) const
{
    float s = mIsUdf(t) ? 0 : (t - sampling.start) / sampling.step;
    return mNINT32(s);
}


SampleGate SeisTrcInfo::sampleGate( const Interval<float>& tg ) const
{
    SampleGate sg;

    sg.start = sg.stop = 0;
    if ( mIsUdf(tg.start) && mIsUdf(tg.stop) )
	return sg;

    Interval<float> vals(
	mIsUdf(tg.start) ? 0 : (tg.start-sampling.start) / sampling.step,
	mIsUdf(tg.stop) ? 0 : (tg.stop-sampling.start) / sampling.step );

    if ( vals.start < vals.stop )
    {
	sg.start = (int)Math::Floor(vals.start+1e-3);
	sg.stop =  (int)Math::Ceil(vals.stop-1e-3);
    }
    else
    {
	sg.start =  (int)Math::Ceil(vals.start-1e-3);
	sg.stop = (int)Math::Floor(vals.stop+1e-3);
    }

    if ( sg.start < 0 ) sg.start = 0;
    if ( sg.stop < 0 ) sg.stop = 0;

    return sg;
}


Seis::PosKey SeisTrcInfo::posKey( Seis::GeomType gt ) const
{
    switch ( gt )
    {
    case Seis::VolPS:	return Seis::PosKey( binID(), offset );
    case Seis::Line:	return Seis::PosKey( nr );
    case Seis::LinePS:	return Seis::PosKey( nr, offset );
    default:		return Seis::PosKey( binID() );
    }
}


void SeisTrcInfo::setPosKey( const Seis::PosKey& pk )
{
    Seis::GeomType gt = pk.geomType();
    if ( Seis::isPS(gt) )
	offset = pk.offset();
    if ( Seis::is2D(gt) )
	nr = pk.trcNr();
    else
	setBinID( pk.binID() );
}


void SeisTrcInfo::setTrcKey( const TrcKey& tk )
{
    if ( tk.is2D() )
	nr = tk.trcNr();
    setBinID( tk.binID() );
}


void SeisTrcInfo::putTo( PosAuxInfo& auxinf ) const
{
    auxinf.binid = binID();
    auxinf.startpos = sampling.start;
    auxinf.coord = coord;
    auxinf.offset = offset;
    auxinf.azimuth = azimuth;
    auxinf.pick = pick;
    auxinf.refnr = refnr;
}


void SeisTrcInfo::getFrom( const PosAuxInfo& auxinf )
{
    setBinID( auxinf.binid );
    sampling.start = auxinf.startpos;
    coord = auxinf.coord;
    offset = auxinf.offset;
    azimuth = auxinf.azimuth;
    pick = auxinf.pick;
    refnr = auxinf.refnr;
}


int Seis::Bounds::expectedNrTraces() const
{
    int rg0 = start(true); int rg1 = stop(true); int delta = step(true);
    int nr1 = (rg1 - rg0) / delta;
    if ( nr1 < 0 )	nr1 -= 1;
    else		nr1 += 1;

    rg0 = start(false);
    if ( mIsUdf(rg0) ) return nr1;

    rg1 = stop(false); delta = step(false);
    int nr2 = (rg1 - rg0) / delta;
    if ( nr2 < 0 )	nr2 -= 1;
    else		nr2 += 1;

    return nr1 * nr2; // Surveys with more than 2G traces ...? Nah.
}


Seis::Bounds3D::Bounds3D()
    : tkzs_(*new TrcKeyZSampling)
{
}


Seis::Bounds3D::Bounds3D( const Bounds3D& b )
    : tkzs_(*new TrcKeyZSampling(b.tkzs_))
{
}


Seis::Bounds3D::~Bounds3D()
{
    delete &tkzs_;
}


int Seis::Bounds3D::start( bool first ) const
{
    return first ? tkzs_.hsamp_.start_.inl() : tkzs_.hsamp_.start_.crl();
}


int Seis::Bounds3D::stop( bool first ) const
{
    return first ? tkzs_.hsamp_.stop_.inl() : tkzs_.hsamp_.stop_.crl();
}


int Seis::Bounds3D::step( bool first ) const
{
    return first ? tkzs_.hsamp_.step_.inl() : tkzs_.hsamp_.step_.crl();
}


StepInterval<float> Seis::Bounds3D::getZRange() const
{
    return tkzs_.zsamp_;
}


void Seis::Bounds3D::getCoordRange( Coord& mn, Coord& mx ) const
{
    mn = SI().transform(
	BinID(tkzs_.hsamp_.start_.inl(),tkzs_.hsamp_.start_.crl()) );
    Coord c = SI().transform(
	BinID(tkzs_.hsamp_.stop_.inl(), tkzs_.hsamp_.start_.crl()) );
    if ( c.x < mn.x ) mn.x = c.x; if ( c.x > mx.x ) mx.x = c.x;
    c = SI().transform(
	BinID(tkzs_.hsamp_.stop_.inl(),tkzs_.hsamp_.stop_.crl()) );
    if ( c.x < mn.x ) mn.x = c.x; if ( c.x > mx.x ) mx.x = c.x;
    c = SI().transform(
	BinID(tkzs_.hsamp_.start_.inl(),tkzs_.hsamp_.stop_.crl()) );
    if ( c.x < mn.x ) mn.x = c.x; if ( c.x > mx.x ) mx.x = c.x;
}


Seis::Bounds2D::Bounds2D()
{
    zrg_ = SI().zRange(false);
    nrrg_.step = 1;
    mincoord_ = SI().minCoord( false );
    maxcoord_ = SI().maxCoord( false );
}


IOObjContext* Seis::getIOObjContext( Seis::GeomType gt, bool forread )
{
    IOObjContext* ret = new IOObjContext( Seis::isPS(gt) ?
	 (Seis::is2D(gt) ? mIOObjContext(SeisPS2D) : mIOObjContext(SeisPS3D))
       : (Seis::is2D(gt) ? mIOObjContext(SeisTrc2D) : mIOObjContext(SeisTrc)) );
    ret->forread_ = forread;
    if ( gt == Seis::Line )
	ret->deftransl_ = CBVSSeisTrc2DTranslator::translKey();
    else if ( gt == Seis::Vol )
	ret->deftransl_ = CBVSSeisTrcTranslator::translKey();
    else if ( gt == Seis::VolPS )
	ret->deftransl_ = CBVSSeisPS3DTranslator::translKey();
    else
	ret->deftransl_ = CBVSSeisPS2DTranslator::translKey();

    return ret;
}
