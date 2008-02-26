/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: posfilter.cc,v 1.5 2008-02-26 08:55:18 cvsbert Exp $";

#include "posfilterset.h"
#include "posfilterstd.h"
#include "posprovider.h"
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
const char* Pos::FilterSet::typeStr() { return "Set"; }
const char* Pos::RandomFilter::typeStr() { return sKey::Random; }
const char* Pos::RandomFilter::ratioStr() { return "Pass ratio"; }
const char* Pos::SubsampFilter::typeStr() { return sKey::Subsample; }
const char* Pos::SubsampFilter::eachStr() { return "Pass each"; }


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
    const char* typ = iop.find(sKey::Type);
    if ( !typ ) return 0;
    Pos::Filter3D* filt = strcmp(typ,Pos::FilterSet::typeStr())
			? factory().create( typ )
			: (Pos::Filter3D*)new Pos::FilterSet3D;
    if ( filt )
	filt->usePar( iop );
    return filt;
}


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


Pos::FilterSet::~FilterSet()
{
    deepErase( *this );
}


void Pos::FilterSet::copyFrom( const Pos::FilterSet& fs )
{
    if ( this != &fs && is2d() == fs.is2d() )
    {
	deepErase( *this );
	for ( int idx=0; idx<fs.size(); idx++ )
	    *this += fs[idx]->clone();
    }
}


void Pos::FilterSet::add( Filter* filt )
{
    if ( !filt || is2d() != filt->is2D() ) return;
    *this += filt;
}


void Pos::FilterSet::add( const IOPar& iop )
{
    Pos::Filter* filt;
    if ( is2d() )
	filt = Pos::Filter2D::make( iop );
    else
	filt = Pos::Filter3D::make( iop );
    if ( filt )
	*this += filt;
}


bool Pos::FilterSet::IMPL_initialize()
{
    for ( int idx=0; idx<size(); idx++ )
	if ( !(*this)[idx]->initialize() )
	    return false;
    return true;
}


Executor* Pos::FilterSet::IMPL_initializer() const
{
    ExecutorGroup* egrp = new ExecutorGroup( "Position filters initializer",
	   				     true );
    for ( int idx=0; idx<size(); idx++ )
    {
	Executor* ex = (*this)[idx]->initializer();
	if ( ex ) egrp->add( ex );
    }

    if ( egrp->nrExecutors() < 1 )
	{ delete egrp; egrp = 0; }
    return egrp;
}


void Pos::FilterSet::IMPL_reset()
{
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx]->reset();
}


bool Pos::FilterSet::IMPL_includes( const Coord& c, float z ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( !(*this)[idx]->includes(c,z) )
	    return false;
    return true;
}


void Pos::FilterSet::IMPL_adjustZ( const Coord& c, float& z ) const
{
    for ( int idx=0; idx<size(); idx++ )
	z = (*this)[idx]->adjustedZ( c, z );
}


bool Pos::FilterSet::IMPL_hasZAdjustment() const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->hasZAdjustment() )
	    return true;
    return false;
}


void Pos::FilterSet::IMPL_fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, "Set" );
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


void Pos::FilterSet::IMPL_usePar( const IOPar& iop )
{
    deepErase( *this );

    for ( int idx=0; ; idx++ )
    {
	const BufferString keybase( IOPar::compKey(sKey::Filter,idx) );
	PtrMan<IOPar> subpar = iop.subselect( keybase );
	if ( !subpar || !subpar->size() ) return;

	Filter* filt = 0;
	if ( is2d() )
	    filt = Pos::Filter2D::make( *subpar );
	else
	    filt = Pos::Filter3D::make( *subpar );

	if ( !filt )
	{
	    if ( is2d() )
		filt = Pos::Provider2D::make( *subpar );
	    else
		filt = Pos::Provider3D::make( *subpar );
	}

	if ( filt )
	    *this += filt;
    }
}


void Pos::FilterSet::IMPL_getSummary( BufferString& txt ) const
{
    if ( isEmpty() ) return;

    if ( size() > 1 ) txt += "{";
    (*this)[0]->getSummary( txt );
    for ( int idx=1; idx<size(); idx++ )
    {
	txt += ",";
	(*this)[idx]->getSummary( txt );
    }
    if ( size() > 1 ) txt += "}";
}


bool Pos::FilterSet3D::includes( const BinID& b, float z ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	Pos::Filter3D* f3d = (Pos::Filter3D*)((*this)[idx]);
	if ( !f3d->includes(b,z) )
	    return false;
    }
    return true;
}


bool Pos::FilterSet2D::includes( int nr, float z ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	Pos::Filter2D* f2d = (Pos::Filter2D*)((*this)[idx]);
	if ( !f2d->includes(nr,z) )
	    return false;
    }
    return true;
}

bool	Pos::FilterSet3D::initialize()
	{ return IMPL_initialize(); }
void	Pos::FilterSet3D::reset()
	{ IMPL_reset(); }
void	Pos::FilterSet3D::fillPar(IOPar& i) const
	{ IMPL_fillPar(i); }
void	Pos::FilterSet3D::usePar(const IOPar& i)
	{ IMPL_usePar(i); }
Executor* Pos::FilterSet3D::initializer() const
	{ return IMPL_initializer(); }
bool	Pos::FilterSet3D::includes(const Coord& c,float z) const
	{ return IMPL_includes(c,z); }
void	Pos::FilterSet3D::adjustZ(const Coord& c,float& z) const
	{ IMPL_adjustZ(c,z); }
bool	Pos::FilterSet3D::hasZAdjustment() const
	{ return IMPL_hasZAdjustment(); }
void	Pos::FilterSet3D::getSummary(BufferString& t) const
	{ IMPL_getSummary(t); }

bool	Pos::FilterSet2D::initialize()
	{ return IMPL_initialize(); }
void	Pos::FilterSet2D::reset()
	{ IMPL_reset(); }
void	Pos::FilterSet2D::fillPar(IOPar& i) const
	{ IMPL_fillPar(i); }
void	Pos::FilterSet2D::usePar(const IOPar& i)
	{ IMPL_usePar(i); }
Executor* Pos::FilterSet2D::initializer() const
	{ return IMPL_initializer(); }
bool	Pos::FilterSet2D::includes(const Coord& c,float z) const
	{ return IMPL_includes(c,z); }
void	Pos::FilterSet2D::adjustZ(const Coord& c,float& z) const
	{ IMPL_adjustZ(c,z); }
bool	Pos::FilterSet2D::hasZAdjustment() const
	{ return IMPL_hasZAdjustment(); }
void	Pos::FilterSet2D::getSummary(BufferString& t) const
	{ IMPL_getSummary(t); }


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


void Pos::SubsampFilter::doUsePar( const IOPar& iop )
{
    iop.get( eachStr(), each_ );
}


void Pos::SubsampFilter::doFillPar( IOPar& iop ) const
{
    iop.set( eachStr(), each_ );
}


void Pos::SubsampFilter::mkSummary( BufferString& txt ) const
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
