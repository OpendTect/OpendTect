/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisinfo.h"

#include "commandlineparser.h"
#include "ctxtioobj.h"
#include "enums.h"
#include "envvars.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_istream.h"
#include "oscommand.h"
#include "posauxinfo.h"
#include "seis2dlineio.h"
#include "seisbounds.h"
#include "seiscbvs.h"
#include "seiscommon.h"
#include "seispacketinfo.h"
#include "seispsioprov.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#include <float.h>
#include <iostream>
#include <math.h>

const char* SeisTrcInfo::sSamplingInfo() { return "Sampling information"; }
const char* SeisTrcInfo::sNrSamples()	{ return "Nr of samples"; }
const char* SeisPacketInfo::sBinIDs()	{ return "BinID range"; }
const char* SeisPacketInfo::sZRange()	{ return "Z range"; }


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


BufferString SeisPacketInfo::defaultusrinfo_;

mStartAllowDeprecatedSection

SeisPacketInfo::SeisPacketInfo()
    : usrinfo(usrinfo_)
    , stdinfo(stdinfo_)
    , nr(nr_)
    , fullyrectandreg(fullyrectandreg_)
    , inlrg(inlrg_)
    , crlrg(crlrg_)
    , zrg(zrg_)
    , inlrev(inlrev_)
    , crlrev(crlrev_)
{
    clear();
}

mStopAllowDeprecatedSection


SeisPacketInfo::~SeisPacketInfo()
{}


void SeisPacketInfo::initClass()
{
    defaultusrinfo_ = getUsrInfo();
}


void SeisPacketInfo::clear()
{
    usrinfo_ = defaultusrinfo_;
    nr_ = 0;
    fullyrectandreg_ = false;
    cubedata_ = nullptr;
    SI().sampling(false).hsamp_.get( inlrg_, crlrg_ );
    zrg_ = SI().zRange(false);
    inlrev_ = false;
    crlrev_ = false;
}



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


void Seis::putInMC( Seis::GeomType gt, OS::MachineCommand& mc )
{
    mc.addKeyedArg( sKey::Geometry(), Seis::nameOf(gt) );
}


bool Seis::getFromCLP( const CommandLineParser& clp, Seis::GeomType& gt )
{
    BufferString res;
    if ( !clp.getVal(sKey::Geometry(),res) || res.isEmpty() )
	return false;

    gt = geomTypeOf( res );
    return true;
}


bool Seis::getFromPar( const IOPar& iop, Seis::GeomType& gt )
{
    const BufferString res = iop.find( sKey::Geometry() );
    if ( res.isEmpty() )
	return false;

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
    const bool both_2d_3d = SI().survDataType()==OD::Both2DAnd3D;
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
	"Sequence number",
	"Geometry ID",
	nullptr
};


mStartAllowDeprecatedSection

SeisTrcInfo::SeisTrcInfo()
    : sampling_(0,defaultSampleInterval())
    , trckey_(*new TrcKey)
    , coord(coord_)
    , sampling(sampling_)
    , offset(offset_)
    , azimuth(azimuth_)
    , refnr(refnr_)
    , pick(pick_)
    , zref(zref_)
    , new_packet(new_packet_)
    , binid(const_cast<BinID&>(trckey_.position()))
    , nr(binid.trcNr())
{
}


SeisTrcInfo::SeisTrcInfo( const SeisTrcInfo& oth )
    : trckey_(*new TrcKey)
    , coord(coord_)
    , sampling(sampling_)
    , offset(offset_)
    , azimuth(azimuth_)
    , refnr(refnr_)
    , pick(pick_)
    , zref(zref_)
    , new_packet(new_packet_)
    , binid(const_cast<BinID&>(trckey_.position()))
    , nr(binid.trcNr())
{
    *this = oth;
}

mStopAllowDeprecatedSection


SeisTrcInfo::~SeisTrcInfo()
{
    delete &trckey_;
}


SeisTrcInfo& SeisTrcInfo::operator=( const SeisTrcInfo& oth )
{
    if ( this != &oth )
    {
	trckey_ = oth.trckey_;
	sampling_ = oth.sampling_;
	coord_ = oth.coord_;
	offset_ = oth.offset_;
	azimuth_ = oth.azimuth_;
	refnr_ = oth.refnr_;
	pick_ = oth.pick_;
	seqnr_ = oth.seqnr_;
	zref_ = oth.zref_;
	new_packet_ = oth.new_packet_;
    }
    return *this;
}


bool SeisTrcInfo::is2D() const
{ return trckey_.is2D(); }
bool SeisTrcInfo::is3D() const
{ return trckey_.is3D(); }
bool SeisTrcInfo::isSynthetic() const
{ return trckey_.isSynthetic();}
OD::GeomSystem SeisTrcInfo::geomSystem() const
{ return trckey_.geomSystem(); }
BinID SeisTrcInfo::binID() const
{ return trckey_.position(); }
Pos::IdxPair SeisTrcInfo::idxPair() const
{ return trckey_.idxPair(); }
SeisTrcInfo::IdxType SeisTrcInfo::inl() const
{ return trckey_.inl(); }
SeisTrcInfo::IdxType SeisTrcInfo::crl() const
{ return trckey_.crl(); }
SeisTrcInfo::IdxType SeisTrcInfo::lineNr() const
{ return trckey_.inl(); }
SeisTrcInfo::IdxType SeisTrcInfo::trcNr() const
{ return trckey_.crl(); }
Pos::GeomID SeisTrcInfo::geomID() const
{ return trckey_.geomID(); }

SeisTrcInfo& SeisTrcInfo::setGeomSystem( OD::GeomSystem gs )
{ trckey_.setGeomSystem( gs ); return *this; }
SeisTrcInfo& SeisTrcInfo::setPos( const BinID& bid )
{ trckey_.setPosition( bid ); return *this; }
SeisTrcInfo& SeisTrcInfo::setPos( Pos::GeomID gid, IdxType tnr )
{ trckey_.setGeomID( gid ).setTrcNr( tnr ); return *this; }
SeisTrcInfo& SeisTrcInfo::setGeomID( Pos::GeomID gid )
{ trckey_.setGeomID( gid ); return *this; }
SeisTrcInfo& SeisTrcInfo::setLineNr( IdxType lnr )
{ trckey_.setLineNr( lnr ); return *this; }
SeisTrcInfo& SeisTrcInfo::setTrcNr( IdxType tnr )
{ trckey_.setTrcNr( tnr ); return *this; }
SeisTrcInfo& SeisTrcInfo::setTrcKey( const TrcKey& tk )
{ trckey_ = tk; return *this; }
SeisTrcInfo& SeisTrcInfo::calcCoord()
{ coord_ = trckey_.getCoord(); return *this; }


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
	case CoordX:	return coord_.x_;
	case CoordY:	return coord_.y_;
    case BinIDInl:	return inl();
    case BinIDCrl:	return crl();
	case Offset:	return offset_;
	case Azimuth:	return azimuth_;
	case RefNr:		return refnr_;
	case Pick:		return pick_;
    case SeqNr:		return seqnr_;
    case GeomID:	return double(geomID().asInt());
    default:		return trcNr();
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


SeisTrcInfo::Fld SeisTrcInfo::getDefaultAxisFld( Seis::GeomType gt,
						 const SeisTrcInfo* next,
						 const SeisTrcInfo* last) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    if ( !next )
	return isps ? Offset : (is2d ? TrcNr : BinIDCrl);

    if ( isps && !Seis::equalOffset(next->offset_,offset_) )
	return Offset;

    if ( last )
    {
	if ( is2d )
	    return lineNr() == last->lineNr() && trcNr() != next->trcNr() ?
								TrcNr : SeqNr;
	else if ( inl() == last->inl() && crl() != next->crl() )
	    return BinIDCrl;
	else if ( crl() == last->crl() && inl() != next->inl() )
	    return BinIDInl;
    }
    else if ( is2d && next->trcNr() != trcNr() )
	return TrcNr;
    else if ( !is2d && next->crl() != crl() )
	return BinIDCrl;
    else if ( !is2d && next->inl() != inl() )
	return BinIDInl;

    // 'normal' doesn't apply, try coordinates
    return mIsZero(next->coord_.x_-coord_.x_,.1) ? CoordY : CoordX;
}


#define mIOIOPar(fn,fld,memb) iopar.fn( FldNames()[(int)fld], memb )

void SeisTrcInfo::getInterestingFlds( Seis::GeomType gt, IOPar& iopar ) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );

    if ( isps )
    {
	mIOIOPar( set, Offset, offset_ );
	mIOIOPar( set, Azimuth, azimuth_ );
    }

    if ( is2d )
    {
	mIOIOPar( set, TrcNr, trcNr() );
	if ( refnr_ && !mIsUdf(refnr_) )
	    mIOIOPar( set, RefNr, refnr_ );
    }
    else
    {
	mIOIOPar( set, BinIDInl, inl() );
	mIOIOPar( set, BinIDCrl, crl() );
	iopar.set( sKey::Position(), binID().toString() );
    }

    mIOIOPar( set, CoordX, coord_.x_ );
    mIOIOPar( set, CoordY, coord_.y_ );

    if ( pick_ && !mIsUdf(pick_) )
	mIOIOPar( set, Pick, pick_ );
    if ( refnr_ && !mIsUdf(refnr_) )
	mIOIOPar( set, RefNr, refnr_ );
}


void SeisTrcInfo::setPSFlds( const Coord& rcv, const Coord& src, bool setpos )
{
    offset_ = (float) rcv.distTo( src );
    azimuth_ = mCast(float, Math::Atan2( rcv.y_ - src.y_, rcv.x_ - src.x_ ) );
    if ( setpos )
    {
	coord_.x_ = .5 * (rcv.x_ + src.x_);
	coord_.y_ = .5 * (rcv.y_ + src.y_);
	setPos( SI().transform(coord_) );
    }
}


void SeisTrcInfo::usePar( const IOPar& iopar )
{
    BinID bid = binID();
    Pos::GeomID gid = geomID();
    int trcnr = trcNr();
    mIOIOPar( get, BinIDInl,	bid.inl() );
    mIOIOPar( get, BinIDCrl,	bid.crl() );
    mIOIOPar( get, GeomID,	gid );
    mIOIOPar( get, TrcNr,	trcnr );
    if ( Survey::isValidGeomID(gid) )
    {
	if ( Survey::is2DGeom(gid) )
	    setGeomID( gid ).setTrcNr( trcnr );
	else
	    setPos( bid );
    }
    else
    {
	pErrMsg("Undefined geomID in IOPar");
	setTrcNr( trcnr );
	setPos( bid );
    }

    mIOIOPar( get, CoordX,	coord_.x_ );
    mIOIOPar( get, CoordY,	coord_.y_ );
    mIOIOPar( get, Offset,	offset_ );
    mIOIOPar( get, Azimuth,	azimuth_ );
    mIOIOPar( get, Pick,	pick_ );
    mIOIOPar( get, RefNr,	refnr_ );
    mIOIOPar( get, SeqNr,	seqnr_ );

    iopar.get( sSamplingInfo(), sampling_.start_, sampling_.step_ );
}


void SeisTrcInfo::fillPar( IOPar& iopar ) const
{
    mIOIOPar( set, GeomID,	geomID() );
    if ( is2D() )
	mIOIOPar( set, TrcNr,	trcNr() );
    else
    {
	mIOIOPar( set, BinIDInl,	inl() );
	mIOIOPar( set, BinIDCrl,	crl() );
    }

    mIOIOPar( set, CoordX,	coord_.x_ );
    mIOIOPar( set, CoordY,	coord_.y_ );
    mIOIOPar( set, Offset,	offset_ );
    mIOIOPar( set, Azimuth,	azimuth_ );
    mIOIOPar( set, Pick,	pick_ );
    mIOIOPar( set, RefNr,	refnr_ );
    mIOIOPar( set, SeqNr,	seqnr_ );

    iopar.set( sSamplingInfo(), sampling_.start_, sampling_.step_ );
}


bool SeisTrcInfo::dataPresent( float t, int trcsz ) const
{
    return t > sampling_.start_-1e-6 && t < samplePos(trcsz-1) + 1e-6;
}


int SeisTrcInfo::nearestSample( float t ) const
{
    float s = mIsUdf(t) ? 0 : (t - sampling_.start_) / sampling_.step_;
    return mNINT32(s);
}


SampleGate SeisTrcInfo::sampleGate( const Interval<float>& tg ) const
{
    SampleGate sg;

    sg.start_ = sg.stop_ = 0;
    if ( mIsUdf(tg.start_) && mIsUdf(tg.stop_) )
	return sg;

    Interval<float> vals(
	mIsUdf(tg.start_) ? 0 : (tg.start_-sampling_.start_) / sampling_.step_,
	mIsUdf(tg.stop_) ? 0 : (tg.stop_-sampling_.start_) / sampling_.step_ );

    if ( vals.start_ < vals.stop_ )
    {
        sg.start_ = (int)Math::Floor(vals.start_+1e-3);
        sg.stop_ =  (int)Math::Ceil(vals.stop_-1e-3);
    }
    else
    {
        sg.start_ =  (int)Math::Ceil(vals.start_-1e-3);
        sg.stop_ = (int)Math::Floor(vals.stop_+1e-3);
    }

    if ( sg.start_ < 0 ) sg.start_ = 0;
    if ( sg.stop_ < 0 ) sg.stop_ = 0;

    return sg;
}


Seis::PosKey SeisTrcInfo::posKey( Seis::GeomType gt ) const
{
    switch ( gt )
    {
	case Seis::VolPS:	return Seis::PosKey( binID(), offset_ );
    case Seis::Line:	return Seis::PosKey( trcNr() );
	case Seis::LinePS:	return Seis::PosKey( trcNr(), offset_ );
    default:		return Seis::PosKey( binID() );
    }
}


void SeisTrcInfo::setPosKey( const Seis::PosKey& pk )
{
    Seis::GeomType gt = pk.geomType();
    if ( Seis::isPS(gt) )
	offset_ = pk.offset();
    if ( Seis::is2D(gt) )
	setTrcNr( pk.trcNr() );
    else
	setPos( pk.binID() );
}


void SeisTrcInfo::putTo( PosAuxInfo& auxinf ) const
{
    auxinf.trckey_ = trckey_;
    auxinf.startpos = sampling_.start_;
    auxinf.coord = coord_;
    auxinf.offset = offset_;
    auxinf.azimuth = azimuth_;
    auxinf.pick = pick_;
    auxinf.refnr = refnr_;
}


void SeisTrcInfo::getFrom( const PosAuxInfo& auxinf )
{
    trckey_ = auxinf.trckey_;
    sampling_.start_ = auxinf.startpos;
    coord_ = auxinf.coord;
    offset_ = auxinf.offset;
    azimuth_ = auxinf.azimuth;
    pick_ = auxinf.pick;
    refnr_ = auxinf.refnr;
}


Seis::Bounds::Bounds()
{}


Seis::Bounds::~Bounds()
{}


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
    if ( c.x_ < mn.x_ ) mn.x_ = c.x_; if ( c.x_ > mx.x_ ) mx.x_ = c.x_;
    c = SI().transform(
	BinID(tkzs_.hsamp_.stop_.inl(),tkzs_.hsamp_.stop_.crl()) );
    if ( c.x_ < mn.x_ ) mn.x_ = c.x_; if ( c.x_ > mx.x_ ) mx.x_ = c.x_;
    c = SI().transform(
	BinID(tkzs_.hsamp_.start_.inl(),tkzs_.hsamp_.stop_.crl()) );
    if ( c.x_ < mn.x_ ) mn.x_ = c.x_; if ( c.x_ > mx.x_ ) mx.x_ = c.x_;
}


Seis::Bounds2D::Bounds2D()
{
    zrg_ = SI().zRange(false);
    nrrg_.step_ = 1;
    mincoord_ = SI().minCoord( false );
    maxcoord_ = SI().maxCoord( false );
}


Seis::Bounds2D::~Bounds2D()
{}


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
