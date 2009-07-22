/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: emsurfaceposprov.cc,v 1.8 2009-07-22 16:01:31 cvsbert Exp $";

#include "emsurfaceposprov.h"

#include "cubesampling.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurface.h"
#include "emsurfaceiodata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

const char* Pos::EMSurfaceProvider::id1Key()		{ return "ID.1"; }
const char* Pos::EMSurfaceProvider::id2Key()		{ return "ID.2"; }
const char* Pos::EMSurfaceProvider::zstepKey()		{ return "Z Step"; }
const char* Pos::EMSurfaceProvider::extraZKey()		{ return "Extra Z"; }


Pos::EMSurfaceProvider::EMSurfaceProvider()
    : surf1_(0)
    , surf2_(0)
    , hs_(SI().sampling(false).hrg)
    , zstep_(SI().zRange(true).step)
    , extraz_(0,0)
    , zrg1_(0,0)
    , zrg2_(0,0)
    , iterator_(0)
    , curz_(mUdf(float))
    , curzrg_(0,0)
{
}


Pos::EMSurfaceProvider::EMSurfaceProvider( const Pos::EMSurfaceProvider& pp )
    : surf1_(0)
    , surf2_(0)
{
    *this = pp;
}


Pos::EMSurfaceProvider::~EMSurfaceProvider()
{
    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
}


void Pos::EMSurfaceProvider::copyFrom( const Pos::EMSurfaceProvider& pp )
{
    if ( &pp == this ) return;

    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
    surf1_ = pp.surf1_; if ( surf1_ ) surf1_->ref();
    surf2_ = pp.surf2_; if ( surf2_ ) surf2_->ref();
    zstep_ = pp.zstep_;
    extraz_ = pp.extraz_;
    id1_ = pp.id1_;
    id2_ = pp.id2_;
    hs_ = pp.hs_;
    zrg1_ = pp.zrg1_; zrg2_ = pp.zrg2_;
}


const char* Pos::EMSurfaceProvider::type() const
{
    return sKey::Surface;
}


static void getSurfRanges( const EM::Surface& surf, HorSampling& hs,
			   Interval<float>& zrg )
{
    bool veryfirst = true;
    for ( int idx=0; idx<surf.nrSections(); idx++ )
    {
	EM::RowColIterator it( surf, surf.sectionID(idx) );
	EM::PosID posid = it.next();
	while ( posid.objectID() != -1 )
	{
	    const Coord3 coord = surf.getPos( posid );
	    const BinID bid( SI().transform(coord) );
	    if ( veryfirst )
	    {
		veryfirst = false;
		hs.start = hs.stop = bid;
		zrg.start = zrg.stop = coord.z;
	    }
	    else
	    {
		if ( coord.z < zrg.start ) zrg.start = coord.z;
		if ( coord.z > zrg.stop ) zrg.stop = coord.z;
		hs.include( bid );
	    }
	    posid = it.next();
	}
    }
}


bool Pos::EMSurfaceProvider::initialize( TaskRunner* tr )
{
    if ( nrSurfaces() == 0 ) return false;

    EM::EMObject* emobj = EM::EMM().getObject( EM::EMM().getObjectID(id1_) );
    if ( !emobj )
	emobj = EM::EMM().loadIfNotFullyLoaded( id1_, tr );
    mDynamicCastGet(EM::Surface*,surf1,emobj)
    if ( !surf1 ) return false;
    surf1_ = surf1; surf1_->ref();

    getSurfRanges( *surf1_, hs_, zrg1_ );

    if ( !id2_.isEmpty() )
    {
	emobj = EM::EMM().getObject( EM::EMM().getObjectID(id2_) );
	if ( !emobj )
	    emobj = EM::EMM().loadIfNotFullyLoaded( id2_, tr );
	mDynamicCastGet(EM::Surface*,surf2,emobj)
	if ( !surf2 ) return false;
	surf2_ = surf2; surf2_->ref();
	HorSampling hs( hs_ );
	getSurfRanges( *surf2_, hs, zrg2_ );
	hs_.limitTo( hs );
    }

    reset();
    return true;
}


void Pos::EMSurfaceProvider::reset()
{
    delete iterator_; iterator_ = 0;
    if ( surf1_ )
	iterator_ = new EM::RowColIterator( *surf1_, surf1_->sectionID(0) );
    curpos_ = EM::PosID( -1, -1, -1 );
}


bool Pos::EMSurfaceProvider::toNextPos()
{
    curpos_ = iterator_->next();
    if ( curpos_.objectID() == -1 )
	return false;

    curzrg_.start = curzrg_.stop = surf1_->getPos( curpos_ ).z;
    if ( surf2_ )
	curzrg_.stop = surf2_->getPos( surf2_->sectionID(0), curpos_.subID()).z;
    curzrg_ += extraz_;

    if ( surf2_ || extraz_.width()>0 )
    {
	SI().snapZ( curzrg_.start, 1 );
	SI().snapZ( curzrg_.stop, -1 );
    }
    curz_ = curzrg_.start;
    return true;
}


bool Pos::EMSurfaceProvider::toNextZ()
{
    if ( mIsUdf(curz_) || !surf2_ )
	return toNextPos();

    curz_ += zstep_;
    if ( curz_ > curzrg_.stop )
	return toNextPos();

    return true;
}


float Pos::EMSurfaceProvider::curZ() const
{ return curz_; }


bool Pos::EMSurfaceProvider::hasZAdjustment() const
{
    return surf1_ && !surf2_;
}


float Pos::EMSurfaceProvider::adjustedZ( const Coord& c, float z ) const
{
    if ( !hasZAdjustment() ) return z;

    const BinID bid = SI().transform( c );
    return surf1_->getPos( surf1_->sectionID(0), bid.getSerialized() ).z;
}


#define mGetSurfKey(k) IOPar::compKey(sKey::Surface,k)


void Pos::EMSurfaceProvider::usePar( const IOPar& iop )
{
    iop.get( mGetSurfKey(id1Key()), id1_ );
    iop.get( mGetSurfKey(id2Key()), id2_ );
    iop.get( mGetSurfKey(zstepKey()), zstep_ );
    iop.get( mGetSurfKey(extraZKey()), extraz_ );

    if ( id1_.isEmpty() ) return;
    EM::SurfaceIOData sd;
    const char* res = EM::EMM().getSurfaceData( id1_, sd );
    if ( res ) return;

    hs_ = sd.rg;

    if ( id2_.isEmpty() ) return;
    res = EM::EMM().getSurfaceData( id2_, sd );
    if ( res ) return;
    hs_.limitTo( sd.rg );
    // TODO: get zrg's
}


void Pos::EMSurfaceProvider::fillPar( IOPar& iop ) const
{
    iop.set( mGetSurfKey(id1Key()), id1_ );
    iop.set( mGetSurfKey(id2Key()), id2_ );
    iop.set( mGetSurfKey(zstepKey()), zstep_ );
    iop.set( mGetSurfKey(extraZKey()), extraz_ );
}


void Pos::EMSurfaceProvider::getSummary( BufferString& txt ) const
{
    switch ( nrSurfaces() )
    {
	case 0:
	return;
	case 1:
	    txt += "On '"; txt += IOM().nameOf(id1_); txt += "'";
	break;
	default:
	    txt += "Between '"; txt += IOM().nameOf(id1_);
	    txt += "' and '"; txt += IOM().nameOf(id2_);
	break;
    }
}


void Pos::EMSurfaceProvider::getZRange( Interval<float>& zrg ) const
{
    if ( !surf1_ ) return;

    zrg = zrg1_;

    if ( surf2_ )
    {
	if ( zrg2_.start < zrg.start ) zrg.start = zrg2_.start;
	if ( zrg2_.stop > zrg.stop ) zrg.stop = zrg2_.stop;
    }
}


int Pos::EMSurfaceProvider::estNrPos() const
{
    return !surf1_ ? 0 : hs_.totalNr();
}


int Pos::EMSurfaceProvider::estNrZPerPos() const
{
    if ( !surf2_ ) return 1;
    Interval<float> avgzrg( (zrg1_.start + zrg2_.start)*.5,
			    (zrg1_.stop + zrg2_.stop)*.5 );
    return (int)((avgzrg.stop-avgzrg.start) / zstep_ + .5);
}


int Pos::EMSurfaceProvider::nrSurfaces() const
{
    if ( !surf1_ )
	return id1_.isEmpty() ? 0 : (id2_.isEmpty() ? 1 : 2);
    return surf2_ ? 2 : 1;
}


BinID Pos::EMSurfaceProvider3D::curBinID() const
{
    BinID bid; bid.setSerialized( curpos_.subID() );
    return bid;
}


bool Pos::EMSurfaceProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !surf1_ )
	return true;

    // TODO: support multiple sections
    Interval<float> zrg;
    const EM::SubID subid = bid.getSerialized();
    const Coord3 crd1 = surf1_->getPos( surf1_->sectionID(0), subid );
    if ( !crd1.isDefined() )
	return false;

    if ( surf2_ )
    {
	const Coord3 crd2 = surf2_->getPos( surf2_->sectionID(0), subid );
	if ( !crd2.isDefined() )
	    return false;

	zrg.start = crd1.z; zrg.stop = crd2.z;
	zrg.sort();
    }
    else
    {
	zrg.start = crd1.z - SI().zStep()/2;
	zrg.stop = crd1.z + SI().zStep()/2;
    }

    zrg += extraz_;
    return zrg.includes( z );
}


void Pos::EMSurfaceProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start; stop = hs_.stop;
}


void Pos::EMSurfaceProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Surface );
}


int Pos::EMSurfaceProvider2D::curNr() const
{
    return 0;
}


Coord Pos::EMSurfaceProvider2D::curCoord() const
{
    return Coord(0,0);
}


bool Pos::EMSurfaceProvider2D::includes( const Coord& c, float z ) const
{
    int nr = 0; // Find nr for this Coord
    return includes( nr, z );
}


bool Pos::EMSurfaceProvider2D::includes( int nr, float z ) const
{
    if ( !surf1_ )
	return false;

    if ( surf2_ )
	return true; // between surf1 and surf2

    return false; // return true when z is within half a Z step
}


void Pos::EMSurfaceProvider2D::getExtent( Interval<int>& intv ) const
{
    intv.start = intv.stop = 0;
}


void Pos::EMSurfaceProvider2D::initClass()
{
    Pos::Provider2D::factory().addCreator( create, sKey::Surface );
}
