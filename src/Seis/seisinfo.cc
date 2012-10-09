/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/

static const char* rcsID = "$Id$";

#include "seisinfo.h"
#include "seispacketinfo.h"
#include "seisbounds.h"
#include "seistrc.h"
#include "posauxinfo.h"
#include "survinfo.h"
#include "strmprov.h"
#include "strmdata.h"
#include "file.h"
#include "iopar.h"
#include "cubesampling.h"
#include "enums.h"
#include "envvars.h"
#include "seistype.h"
#include "keystrs.h"
#include "timeser.h"

#include <float.h>
#include <iostream>
#include <math.h>

const char* SeisTrcInfo::sSamplingInfo = "Sampling information";
const char* SeisTrcInfo::sNrSamples = "Nr of samples";
const char* SeisPacketInfo::sBinIDs = "BinID range";
const char* SeisPacketInfo::sZRange = "Z range";


static BufferString getUsrInfo()
{
    BufferString bs;
    const char* envstr = GetEnvVar( "DTECT_SEIS_USRINFO" );
    if ( !envstr || !File::exists(envstr) ) return bs;

    StreamData sd = StreamProvider(envstr).makeIStream();
    if ( sd.usable() )
    {
	char buf[1024];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 1024 );
	    if ( *(const char*)bs ) bs += "\n";
	    bs += buf;
	}
    }

    return bs;
}

BufferString SeisPacketInfo::defaultusrinfo = getUsrInfo();

class SeisEnum
{
public:
    typedef Seis::SelType SelType;
	    DeclareEnumUtils(SelType)
    typedef Seis::GeomType GeomType;
	    DeclareEnumUtils(GeomType)
    typedef Seis::WaveType WaveType;
	    DeclareEnumUtils(WaveType)
    typedef Seis::DataType DataType;
	    DeclareEnumUtils(DataType)
};

DefineEnumNames(SeisEnum,SelType,0,"Selection type")
{
	sKey::Range,
	sKey::Table,
	sKey::Polygon,
	0
};

DefineEnumNames(SeisEnum,GeomType,0,"Geometry type")
{
	"3D Volume",
	"Pre-Stack Volume",
	"2D Line",
	"Line 2D Pre-Stack",
	0
};

DefineEnumNames(SeisEnum,WaveType,0,"Wave type")
{
	"P",
	"Sh",
	"Sv",
	"Other",
	0
};

DefineEnumNames(SeisEnum,DataType,0,"Data type")
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
    if ( s && !strcmp(s,sKey::Steering) )
	return Seis::Dip;
    SeisEnum::DataType res; SeisEnum::parseEnumDataType(s,res); return res;
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
    iop.set( sKey::Geometry, Seis::nameOf(gt) );
}

bool Seis::getFromPar( const IOPar& iop, Seis::GeomType& gt )
{
    const char* res = iop.find( sKey::Geometry );
    if ( !res || !*res ) return false;
    gt = geomTypeOf( res );
    return true;
}
 

DefineEnumNames(SeisTrcInfo,Fld,1,"Header field") {
	"Trace number",
	"Pick position",
	"Reference position",
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
    SI().sampling(false).hrg.get( inlrg, crlrg );
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
    return ival * 0.001;
}


double SeisTrcInfo::getValue( SeisTrcInfo::Fld fld ) const
{
    switch ( fld )
    {
    case CoordX:	return coord.x;
    case CoordY:	return coord.y;
    case BinIDInl:	return binid.inl;
    case BinIDCrl:	return binid.crl;
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

    if ( Seis::isPS(gt) )
	{ flds += Offset; flds += Azimuth; }
    else if ( Seis::is2D(gt) )
	{ flds += TrcNr; flds += RefNr; }
    else
	{ flds += BinIDInl; flds += BinIDCrl; }

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

    if ( isps && !mIsZero(ti->offset-offset,1e-4) )
	return Offset;
    if ( is2d && ti->nr != nr )
	return TrcNr;
    if ( !is2d && ti->binid.crl != binid.crl )
	return BinIDCrl;
    if ( !is2d && ti->binid.inl != binid.inl )
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
	mIOIOPar( set, BinIDInl, binid.inl );
	mIOIOPar( set, BinIDCrl, binid.crl );
	BufferString str( 120, false ); binid.fill( str.buf() );
	iopar.set( sKey::Position, str );
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
    offset = rcv.distTo( src );
    azimuth = atan2( rcv.y - src.y, rcv.x - src.x );
    if ( setpos )
    {
	coord.x = .5 * (rcv.x + src.x);
	coord.y = .5 * (rcv.y + src.y);
	binid = SI().transform( coord );
    }
}


void SeisTrcInfo::usePar( const IOPar& iopar )
{
    mIOIOPar( get, TrcNr,	nr );
    mIOIOPar( get, BinIDInl,	binid.inl );
    mIOIOPar( get, BinIDCrl,	binid.crl );
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
    mIOIOPar( set, BinIDInl,	binid.inl );
    mIOIOPar( set, BinIDCrl,	binid.crl );
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
	sg.start = (int)floor(vals.start+1e-3);
	sg.stop =  (int)ceil(vals.stop-1e-3);
    }
    else
    {
	sg.start =  (int)ceil(vals.start-1e-3);
	sg.stop = (int)floor(vals.stop+1e-3);
    }

    if ( sg.start < 0 ) sg.start = 0;
    if ( sg.stop < 0 ) sg.stop = 0;

    return sg;
}


Seis::PosKey SeisTrcInfo::posKey( Seis::GeomType gt ) const
{
    switch ( gt )
    {
    case Seis::VolPS:	return Seis::PosKey( binid, offset );
    case Seis::Line:	return Seis::PosKey( nr );
    case Seis::LinePS:	return Seis::PosKey( nr, offset );
    default: 		return Seis::PosKey( binid );
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
	binid = pk.binID();
}


void SeisTrcInfo::putTo( PosAuxInfo& auxinf ) const
{
    auxinf.binid = binid;
    auxinf.startpos = sampling.start;
    auxinf.coord = coord;
    auxinf.offset = offset;
    auxinf.azimuth = azimuth;
    auxinf.pick = pick;
    auxinf.refnr = refnr;
}


void SeisTrcInfo::getFrom( const PosAuxInfo& auxinf )
{
    binid = auxinf.binid;
    sampling.start = auxinf.startpos;
    coord = auxinf.coord;
    offset = auxinf.offset;
    azimuth = auxinf.azimuth;
    pick = auxinf.pick;
    refnr = auxinf.refnr;
}


void SeisTrcInfo::handlePossibleFeetConversion( bool conv_back,
						bool othdomain )
{
    if ( SI().zIsTime() != othdomain		// data is in time
      || (othdomain && !SI().depthsInFeetByDefault())
      || (!othdomain && !SI().zInFeet()) )	// data needs to stay in meters
	return;

    const float fac = conv_back ? mToFeetFactor : mFromFeetFactor;

    sampling.scale( fac );
    if ( !mIsUdf(pick) ) pick *= fac;
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
    : cs_(*new CubeSampling)
{
}


Seis::Bounds3D::Bounds3D( const Bounds3D& b )
    : cs_(*new CubeSampling(b.cs_))
{
}


Seis::Bounds3D::~Bounds3D()
{
    delete &cs_;
}


int Seis::Bounds3D::start( bool first ) const
{
    return first ? cs_.hrg.start.inl : cs_.hrg.start.crl;
}


int Seis::Bounds3D::stop( bool first ) const
{
    return first ? cs_.hrg.stop.inl : cs_.hrg.stop.crl;
}


int Seis::Bounds3D::step( bool first ) const
{
    return first ? cs_.hrg.step.inl : cs_.hrg.step.crl;
}


StepInterval<float> Seis::Bounds3D::getZRange() const
{
    return cs_.zrg;
}


void Seis::Bounds3D::getCoordRange( Coord& mn, Coord& mx ) const
{
    mn = SI().transform( BinID(cs_.hrg.start.inl,cs_.hrg.start.crl) );
    Coord c = SI().transform( BinID(cs_.hrg.stop.inl,cs_.hrg.start.crl) );
    if ( c.x < mn.x ) mn.x = c.x; if ( c.x > mx.x ) mx.x = c.x;
    c = SI().transform( BinID(cs_.hrg.stop.inl,cs_.hrg.stop.crl) );
    if ( c.x < mn.x ) mn.x = c.x; if ( c.x > mx.x ) mx.x = c.x;
    c = SI().transform( BinID(cs_.hrg.start.inl,cs_.hrg.stop.crl) );
    if ( c.x < mn.x ) mn.x = c.x; if ( c.x > mx.x ) mx.x = c.x;
}


Seis::Bounds2D::Bounds2D()
{
    zrg_ = SI().zRange(false);
    nrrg_.step = 1;
    mincoord_ = SI().minCoord( false );
    maxcoord_ = SI().maxCoord( false );
}
