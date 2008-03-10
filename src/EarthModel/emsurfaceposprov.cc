/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: emsurfaceposprov.cc,v 1.3 2008-03-10 16:35:14 cvsbert Exp $";

#include "emsurfaceposprov.h"
#include "emsurface.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "cubesampling.h"

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


bool Pos::EMSurfaceProvider::initialize( TaskRunner* tr )
{
    if ( nrSurfaces() == 0 ) return 0;
    // Read surface(s) here
    // EMM().loadIfNotFullyLoaded
    return 0;
}


void Pos::EMSurfaceProvider::reset()
{
}


bool Pos::EMSurfaceProvider::toNextPos()
{
    return false;
}


bool Pos::EMSurfaceProvider::toNextZ()
{
    if ( !surf2_ ) return toNextPos();

    return true;
}


float Pos::EMSurfaceProvider::curZ() const
{
    return 0;
}


bool Pos::EMSurfaceProvider::hasZAdjustment() const
{
    return surf1_ && !surf2_;
}


float Pos::EMSurfaceProvider::adjustedZ( const Coord& c, float z ) const
{
    if ( !hasZAdjustment() ) return z;
    // return Z on surface
    return z;
}


#define mGetSurfKey(k) IOPar::compKey(sKey::Surface,k)


void Pos::EMSurfaceProvider::usePar( const IOPar& iop )
{
    iop.get( mGetSurfKey(id1Key()), id1_ );
    iop.get( mGetSurfKey(id2Key()), id2_ );
    iop.get( mGetSurfKey(zstepKey()), zstep_ );
    iop.get( mGetSurfKey(extraZKey()), extraz_ );

    /*
    if ( id1.isEmpty() ) return;
    PtrMan<IOObj> ioobj = IOM().get( id1 );
    if ( !ioobj ) return;
    PtrMan<IOPar> par = EM::EMM().getSurfacePars( *ioobj );
    if ( !par || !par->size() ) return;



    if ( id2.isEmpty() ) return;
    // Here read header(s) to get hs_ and zrg's
    */
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
    return BinID(0,0);
}


bool Pos::EMSurfaceProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !surf1_ )
	return true;

    if ( surf2_ )
	return true; // between surf1 and surf2

    return false; // return true when z is within half a Z step
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
