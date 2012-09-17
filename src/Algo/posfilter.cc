/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: posfilter.cc,v 1.15 2011/11/14 07:39:14 cvssatyaki Exp $";

#include "cubesampling.h"
#include "executor.h"
#include "iopar.h"
#include "keystrs.h"
#include "posfilterset.h"
#include "posfilterstd.h"
#include "posprovider.h"
#include "survinfo.h"
#include "statrand.h"

mImplFactory(Pos::Filter3D,Pos::Filter3D::factory);
mImplFactory(Pos::Filter2D,Pos::Filter2D::factory);
mImplFactory(Pos::Provider3D,Pos::Provider3D::factory);
mImplFactory(Pos::Provider2D,Pos::Provider2D::factory);
const char* Pos::FilterSet::typeStr() { return "Set"; }
const char* Pos::RandomFilter::typeStr() { return sKey::Random; }
const char* Pos::RandomFilter::ratioStr() { return "Pass ratio"; }
const char* Pos::SubsampFilter::typeStr() { return sKey::Subsample; }
const char* Pos::SubsampFilter::eachStr() { return "Pass each"; }


Pos::Filter* Pos::Filter::make( const IOPar& iop, bool is2d )
{
    if ( is2d )
       return Pos::Filter2D::make(iop);
    else
       return Pos::Filter3D::make(iop);
}


bool Pos::Filter3D::includes( const Coord& c, float z ) const
{
    return includes( SI().transform(c), z );
}


Pos::Filter3D* Pos::Filter3D::make( const IOPar& iop )
{
    const char* typ = iop.find(sKey::Type);
    if ( !typ ) return 0;
    Pos::Filter3D* filt = strcmp(typ,Pos::FilterSet::typeStr())
			? factory().create( typ )
			: (Pos::Filter3D*)new Pos::FilterSet3D;
    if ( filt )
	filt->usePar( iop );
    return filt;
}


Pos::Filter2D::~Filter2D()
{
    geomids_.erase(); 
}


int Pos::Filter2D::nrLines() const
{ return geomids_.size(); }


Pos::Filter2D* Pos::Filter2D::make( const IOPar& iop )
{
    const char* typ = iop.find(sKey::Type);
    if ( !typ ) return 0;
    Pos::Filter2D* filt = strcmp(typ,Pos::FilterSet::typeStr())
			? factory().create( typ )
			: (Pos::Filter2D*)new Pos::FilterSet2D;
    if ( filt )
	filt->usePar( iop );
    return filt;
}


void Pos::Filter2D::addLineID( const PosInfo::GeomID& geomid )
{ geomids_ += geomid; }


void Pos::Filter2D::removeLineID( int lidx )
{
    if ( geomids_.validIdx(lidx) )
	geomids_.remove( lidx );
}


PosInfo::GeomID Pos::Filter2D::lineID( int lidx ) const
{ return geomids_.validIdx(lidx) ? geomids_[lidx] : PosInfo::GeomID(); }


Pos::FilterSet::~FilterSet()
{
    deepErase( filts_ );
}


void Pos::FilterSet::copyFrom( const Pos::FilterSet& fs )
{
    if ( this != &fs && is2D() == fs.is2D() )
    {
	deepErase( filts_ );
	for ( int idx=0; idx<fs.size(); idx++ )
	    filts_ += fs.filts_[idx]->clone();
    }
}


void Pos::FilterSet::add( Filter* filt )
{
    if ( !filt || is2D() != filt->is2D() ) return;
    filts_ += filt;
}


void Pos::FilterSet::add( const IOPar& iop )
{
    Pos::Filter* filt = Pos::Filter::make( iop, is2D() );
    if ( filt )
	filts_ += filt;
}


bool Pos::FilterSet::initialize( TaskRunner* tr )
{
    for ( int idx=0; idx<size(); idx++ )
	if ( !filts_[idx]->initialize( tr ) )
	    return false;
    return true;
}


void Pos::FilterSet::reset()
{
    for ( int idx=0; idx<size(); idx++ )
	filts_[idx]->reset();
}


bool Pos::FilterSet::includes( const Coord& c, float z ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( !filts_[idx]->includes(c,z) )
	    return false;
    return true;
}


float Pos::FilterSet::adjustedZ( const Coord& c, float z ) const
{
    for ( int idx=0; idx<size(); idx++ )
	z = filts_[idx]->adjustedZ( c, z );
    return z;
}


bool Pos::FilterSet::hasZAdjustment() const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( filts_[idx]->hasZAdjustment() )
	    return true;
    return false;
}


void Pos::FilterSet::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, "Set" );
    for ( int idx=0; idx<size(); idx++ )
    {
	const Filter& filt = *filts_[idx];
	IOPar filtpar;
	filtpar.set( sKey::Type, filt.type() );
	filt.fillPar( filtpar );
	const BufferString keybase( IOPar::compKey(sKey::Filter,idx) );
	iop.mergeComp( filtpar, keybase );
    }
}


void Pos::FilterSet::usePar( const IOPar& iop )
{
    deepErase( filts_ );

    for ( int idx=0; ; idx++ )
    {
	const BufferString keybase( IOPar::compKey(sKey::Filter,idx) );
	PtrMan<IOPar> subpar = iop.subselect( keybase );
	if ( !subpar || !subpar->size() ) return;

	Pos::Filter* filt = Pos::Filter::make( *subpar, is2D() );
	if ( !filt )
	    filt = Pos::Provider::make( *subpar, is2D() );

	if ( filt )
	    filts_ += filt;
    }
}


void Pos::FilterSet::getSummary( BufferString& txt ) const
{
    if ( isEmpty() )
	{ txt += "-"; return; }

    if ( size() > 1 ) txt += "{";
    filts_[0]->getSummary( txt );
    for ( int idx=1; idx<size(); idx++ )
    {
	txt += ",";
	filts_[idx]->getSummary( txt );
    }
    if ( size() > 1 ) txt += "}";
}


float Pos::FilterSet::estRatio( const Pos::Provider& prov ) const
{
    float ratio = 1;
    for ( int idx=0; idx<size(); idx++ )
	ratio *= filts_[idx]->estRatio( prov );
    return ratio;
}


bool Pos::FilterSet3D::includes( const BinID& b, float z ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(const Pos::Filter3D*,f3d,filts_[idx])
	if ( !f3d->includes(b,z) )
	    return false;
    }
    return true;
}


bool Pos::FilterSet2D::includes( int nr, float z, int lidx ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(const Pos::Filter2D*,f2d,filts_[idx])
	if ( !f2d->includes(nr,z,lidx) )
	    return false;
    }
    return true;
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


void Pos::RandomFilter::usePar( const IOPar& iop )
{
    iop.get( ratioStr(), passratio_ );
}


void Pos::RandomFilter::fillPar( IOPar& iop ) const
{
    iop.set( ratioStr(), passratio_ );
}


void Pos::RandomFilter::getSummary( BufferString& txt ) const
{
    txt += "Remove " ; txt += (1-passratio_)*100; txt += "%";
}


void Pos::RandomFilter3D::initClass()
{
    Pos::Filter3D::factory().addCreator( create, sKey::Random );
}


void Pos::RandomFilter2D::initClass()
{
    Pos::Filter2D::factory().addCreator( create, sKey::Random );
}


bool Pos::SubsampFilter::drawRes() const
{
    seqnr_++;
    return seqnr_ % each_ == 0;
}


void Pos::SubsampFilter::usePar( const IOPar& iop )
{
    iop.get( eachStr(), each_ );
}


void Pos::SubsampFilter::fillPar( IOPar& iop ) const
{
    iop.set( eachStr(), each_ );
}


void Pos::SubsampFilter::getSummary( BufferString& txt ) const
{
    txt += "Pass each " ; txt += each_; txt += getRankPostFix(each_);
}


void Pos::SubsampFilter3D::initClass()
{
    Pos::Filter3D::factory().addCreator( create, sKey::Subsample );
}


void Pos::SubsampFilter2D::initClass()
{
    Pos::Filter2D::factory().addCreator( create, sKey::Subsample );
}


float Pos::Provider::estRatio( const Pos::Provider& prov ) const
{
    if ( is2D() )
	return ( prov.estNrPos()*prov.estNrZPerPos() )/
	       ( estNrPos() * estNrZPerPos() );
    else
    {
	mDynamicCastGet(const Pos::Provider3D*,prov3d,&prov);
	if ( !prov3d ) return mUdf(float);
	CubeSampling provcs( true ); prov3d->getCubeSampling( provcs );
	float provnr = provcs.hrg.totalNr(); provnr *= provcs.zrg.nrSteps() + 1;
	return ( provnr / estNrPos() ) / estNrZPerPos();
    }
}


void Pos::Provider::getCubeSampling( CubeSampling& cs ) const
{
    if ( is2D() )
    {
	cs.set2DDef();
	mDynamicCastGet(const Pos::Provider2D*,prov2d,this)
	StepInterval<int> ext; prov2d->getExtent( ext, 0 );
	cs.hrg.start.crl = ext.start; cs.hrg.stop.crl = ext.stop;
	cs.hrg.step.crl = ext.step;
	prov2d->getZRange( cs.zrg, 0 );
    }
    else
    {
	cs = CubeSampling(true);
	mDynamicCastGet(const Pos::Provider3D*,prov3d,this)
	prov3d->getExtent( cs.hrg.start, cs.hrg.stop );
	prov3d->getZRange( cs.zrg );
    }

}


bool Pos::Provider3D::includes( const Coord& c, float z ) const
{
    return includes( SI().transform(c), z );
}


Coord Pos::Provider3D::curCoord() const
{
    return SI().transform( curBinID() );
}


Pos::Provider* Pos::Provider::make( const IOPar& iop, bool is2d )
{
    if ( is2d )
       return Pos::Provider2D::make(iop);
    else
       return Pos::Provider3D::make(iop);
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
