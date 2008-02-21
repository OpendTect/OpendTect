/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: posfilter.cc,v 1.1 2008-02-21 15:29:10 cvsbert Exp $";

#include "posprovider.h"
#include "posrandomfilter.h"
#include "survinfo.h"
#include "executor.h"
#include "iopar.h"
#include "keystrs.h"
#include "statrand.h"
#include "cubesampling.h"

mImplFactory(Pos::Filter3D,Pos::Filter3D::factory);
mImplFactory(Pos::Filter2D,Pos::Filter2D::factory);
mImplFactory(Pos::Provider3D,Pos::Provider3D::factory);
mImplFactory(Pos::Provider2D,Pos::Provider2D::factory);
const char* Pos::RandomFilter::typeStr() { return sKey::Random; }
const char* Pos::RandomFilter::ratioStr() { return "Pass ratio"; }


bool Pos::Filter::initialize()
{
    Executor* exec = initializer();
    if ( exec ) return exec->execute();
    reset(); return true;
}


bool Pos::Filter3D::includes( const Coord& c, float z ) const
{
    return includes( SI().transform(c), z );
}


Pos::Filter3D* Pos::Filter3D::make( const IOPar& iop )
{
    Pos::Filter3D* filt = factory().create( iop.find(sKey::Type) );
    if ( filt )
	filt->usePar( iop );
    return filt;
}


Pos::Filter2D* Pos::Filter2D::make( const IOPar& iop )
{
    Pos::Filter2D* filt = factory().create( iop.find(sKey::Type) );
    if ( filt )
	filt->usePar( iop );
    return filt;
}


Pos::FilterSet::~FilterSet()
{
    deepErase( *this );
}


Pos::FilterSet& Pos::FilterSet::operator =( const Pos::FilterSet& fs )
{
    if ( this != &fs )
    {
	is2d_ = fs.is2d_;
	deepErase( *this );
	for ( int idx=0; idx<fs.size(); idx++ )
	    *this += fs[idx]->clone();
    }
    return *this;
}


bool Pos::FilterSet::includes( const Coord& c, float z ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( !(*this)[idx]->includes(c) )
	    return false;
    return true;
}


bool Pos::FilterSet::includes( const BinID& b, float z ) const
{
    if ( is2d_ )
	return includes( b.crl, z );

    for ( int idx=0; idx<size(); idx++ )
    {
	Pos::Filter3D* f3d = (Pos::Filter3D*)((*this)[idx]);
	if ( !f3d->includes(b,z) )
	    return false;
    }
    return true;
}


bool Pos::FilterSet::includes( int nr, float z ) const
{
    if ( !is2d_ ) return false;

    for ( int idx=0; idx<size(); idx++ )
    {
	Pos::Filter2D* f2d = (Pos::Filter2D*)((*this)[idx]);
	if ( !f2d->includes(nr,z) )
	    return false;
    }
    return true;
}


void Pos::FilterSet::add( Filter* filt )
{
    if ( !filt || is2d_ != filt->is2D() ) return;
    *this += filt;
}


void Pos::FilterSet::add( const IOPar& iop )
{
    Pos::Filter* filt;
    if ( is2d_ )
	filt = Pos::Filter2D::make( iop );
    else
	filt = Pos::Filter3D::make( iop );
    if ( filt )
	*this += filt;
}


void Pos::FilterSet::adjustZ( const Coord& c, float& z ) const
{
    for ( int idx=0; idx<size(); idx++ )
	z = (*this)[idx]->adjustedZ( c, z );
}


void Pos::FilterSet::usePar( const IOPar& iop )
{
    deepErase( *this );

    for ( int idx=0; ; idx++ )
    {
	const BufferString keybase( IOPar::compKey(sKey::Filter,idx) );
	PtrMan<IOPar> subpar = iop.subselect( keybase );
	if ( !subpar || !subpar->size() ) return;

	const char* typ = subpar->find( sKey::Type );
	Filter* filt = 0;
	if ( is2d_ )
	    filt = Pos::Filter2D::make( typ );
	else
	    filt = Pos::Filter3D::make( typ );

	if ( !filt )
	{
	    if ( is2d_ )
		filt = Pos::Provider2D::make( typ );
	    else
		filt = Pos::Provider3D::make( typ );
	}
	if ( !filt ) continue;

	filt->usePar( *subpar );
	*this += filt;
    }
}


void Pos::FilterSet::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Filter& filt = *(*this)[idx];
	IOPar filtpar;
	filtpar.set( sKey::Type, filt.type() );
	filt.fillPar( filtpar );
	const BufferString keybase( IOPar::compKey(sKey::Filter,idx) );
	iop.mergeComp( filtpar, keybase );
    }
}


void Pos::RandomFilter::initStats()
{
    Stats::RandGen::init();
    if ( passratio_ > 1 ) passratio_ /= 100;
}


bool Pos::RandomFilter::drawRes() const
{
    return Stats::RandGen::get() < passratio_;
}


void Pos::RandomFilter::doUsePar( const IOPar& iop )
{
    iop.get( ratioStr(), passratio_ );
}


void Pos::RandomFilter::doFillPar( IOPar& iop ) const
{
    iop.set( ratioStr(), passratio_ );
}


void Pos::RandomFilter::mkSummary( BufferString& txt ) const
{
    txt += "remove " ; (1-passratio_)*100; txt += "%";
}


void Pos::RandomFilter3D::initClass()
{
    Pos::Filter3D::factory().addCreator( create, sKey::Random );
}


void Pos::RandomFilter2D::initClass()
{
    Pos::Filter2D::factory().addCreator( create, sKey::Random );
}


void Pos::Provider::getCubeSampling( CubeSampling& cs ) const
{
    if ( is2D() )
    {
	cs.set2DDef();
	mDynamicCastGet(const Pos::Provider2D*,prov2d,this)
	StepInterval<int> ext; prov2d->getExtent( ext );
	cs.hrg.start.crl = ext.start; cs.hrg.stop.crl = ext.stop;
	cs.hrg.step.crl = ext.step;
    }
    else
    {
	cs = CubeSampling(true);
	mDynamicCastGet(const Pos::Provider3D*,prov3d,this)
	prov3d->getExtent( cs.hrg.start, cs.hrg.stop );
    }

    getZRange( cs.zrg );
}


bool Pos::Provider3D::includes( const Coord& c, float z ) const
{
    return includes( SI().transform(c), z );
}


Coord Pos::Provider3D::curCoord() const
{
    return SI().transform( curBinID() );
}


Pos::Provider3D* Pos::Provider3D::make( const IOPar& iop )
{
    Pos::Provider3D* prov = factory().create( iop.find(sKey::Type) );
    if ( prov )
	prov->usePar( iop );
    return prov;
}


Pos::Provider2D* Pos::Provider2D::make( const IOPar& iop )
{
    Pos::Provider2D* prov = factory().create( iop.find(sKey::Type) );
    if ( prov )
	prov->usePar( iop );
    return prov;
}
