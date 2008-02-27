/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: emsurfaceposprov.cc,v 1.2 2008-02-27 13:42:08 cvsbert Exp $";

#include "emsurfaceposprov.h"
#include "emsurface.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

const char* Pos::EMSurfaceProvider3D::id1Key()		{ return "ID.1"; }
const char* Pos::EMSurfaceProvider3D::id2Key()		{ return "ID.2"; }
const char* Pos::EMSurfaceProvider3D::zstepKey()	{ return "Z Step"; }


Pos::EMSurfaceProvider3D::EMSurfaceProvider3D()
    : surf1_(0)
    , surf2_(0)
    , zstep_(SI().zRange(true).step)
{
}


Pos::EMSurfaceProvider3D::EMSurfaceProvider3D(
	const Pos::EMSurfaceProvider3D& pp )
    : surf1_(0)
    , surf2_(0)
{
    *this = pp;
}


Pos::EMSurfaceProvider3D::~EMSurfaceProvider3D()
{
    // delete surf1_; or whatever you do for EM objects
    // delete surf2_;
}


Pos::EMSurfaceProvider3D& Pos::EMSurfaceProvider3D::operator =(
	const Pos::EMSurfaceProvider3D& pp )
{
    if ( &pp != this )
    {
	// delete surf1_; delete surf2_;
	// surf1_ = pp.surf1_ ? pp.surf1_->clone() : 0;
	// surf2_ = pp.surf2_ ? pp.surf2_->clone() : 0;
	id1_ = pp.id1_; id2_ = pp.id2_;
	zstep_ = pp.zstep_;
	hs_ = pp.hs_;
	zrg1_ = pp.zrg1_; zrg2_ = pp.zrg2_;
    }
    return *this;
}


int Pos::EMSurfaceProvider3D::nrSurfaces() const
{
    if ( surf1_ ) return surf2_ ? 2 : 1;
    return id1_.isEmpty() ? 0 : (id2_.isEmpty() ? 1 : 2);
}


const char* Pos::EMSurfaceProvider3D::type() const
{
    return sKey::Surface;
}


Executor* Pos::EMSurfaceProvider3D::initializer()
{
    if ( nrSurfaces() == 0 ) return 0;
    // Read surfaces here
    return 0;
}


void Pos::EMSurfaceProvider3D::reset()
{
}


bool Pos::EMSurfaceProvider3D::toNextPos()
{
    return false;
}


bool Pos::EMSurfaceProvider3D::toNextZ()
{
    if ( !surf2_ ) return toNextPos();

    return true;
}


BinID Pos::EMSurfaceProvider3D::curBinID() const
{
    return BinID(0,0);
}


float Pos::EMSurfaceProvider3D::curZ() const
{
    return 0;
}


bool Pos::EMSurfaceProvider3D::includes( const BinID& bid, float z ) const
{
    return false;
}


bool Pos::EMSurfaceProvider3D::hasZAdjustment() const
{
    return surf1_ && !surf2_;
}


float Pos::EMSurfaceProvider3D::adjustedZ( const Coord& c, float z ) const
{
    if ( !hasZAdjustment() ) return z;
    // return Z on surface
    return z;
}


#define mGetSurfKey(k) IOPar::compKey(sKey::Surface,k)


void Pos::EMSurfaceProvider3D::usePar( const IOPar& iop )
{
    iop.get( mGetSurfKey(id1Key()), id1_ );
    iop.get( mGetSurfKey(id2Key()), id2_ );
    iop.get( mGetSurfKey(zstepKey()), zstep_ );
    // Here read header(s) to get hs_ and zrg's
}


void Pos::EMSurfaceProvider3D::fillPar( IOPar& iop ) const
{
    iop.set( mGetSurfKey(id1Key()), id1_ );
    iop.set( mGetSurfKey(id2Key()), id2_ );
    iop.set( mGetSurfKey(zstepKey()), zstep_ );
}


void Pos::EMSurfaceProvider3D::getSummary( BufferString& txt ) const
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


void Pos::EMSurfaceProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start; stop = hs_.stop;
}


void Pos::EMSurfaceProvider3D::getZRange( Interval<float>& zrg ) const
{
    if ( !surf1_ ) return;

    zrg = zrg1_;

    if ( surf2_ )
    {
	if ( zrg2_.start < zrg.start ) zrg.start = zrg2_.start;
	if ( zrg2_.stop > zrg.stop ) zrg.stop = zrg2_.stop;
    }
}


int Pos::EMSurfaceProvider3D::estNrPos() const
{
    return !surf1_ ? 0 : hs_.totalNr();
}


int Pos::EMSurfaceProvider3D::estNrZPerPos() const
{
    if ( !surf2_ ) return 1;
    Interval<float> avgzrg( (zrg1_.start + zrg2_.start)*.5,
			    (zrg1_.stop + zrg2_.stop)*.5 );
    return (int)((avgzrg.stop-avgzrg.start) / zstep_ + .5);
}


void Pos::EMSurfaceProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Surface );
}
