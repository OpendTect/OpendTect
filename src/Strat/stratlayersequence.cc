/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlayersequence.h"

#include "stratlayer.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"


//------ LayerSequence ------

Strat::LayerSequence::LayerSequence( const PropertyRefSelection* prs )
{
    if ( prs )
	props_ = *prs;
}


Strat::LayerSequence::LayerSequence( const Strat::LayerSequence& ls )
{
    *this = ls;
}


Strat::LayerSequence::~LayerSequence()
{
    deepErase( layers_ );
}


Strat::LayerSequence& Strat::LayerSequence::operator =(
					const LayerSequence& oth )
{
    if ( this != &oth )
    {
	deepCopy( layers_, oth.layers_ );
	z0_ = oth.z0_;
	velabove_ = oth.velabove_;
	props_ = oth.props_;
    }

    return *this;
}


bool Strat::LayerSequence::isEmpty() const
{
    return layers_.isEmpty();
}


void Strat::LayerSequence::setEmpty()
{
    deepErase( layers_ );
}


int Strat::LayerSequence::size() const
{
    return layers_.size();
}


void Strat::LayerSequence::setXPos( float xpos )
{
    const int nrlays = layers_.size();
    for ( int ilay=0; ilay<nrlays; ilay++ )
	layers_[ilay]->setXPos( xpos );
}


void Strat::LayerSequence::setStartDepth( float z, bool only, bool ajustlayers )
{
    if ( mIsEqual(z,z0_,1e-2f) )
	return;

    z0_ = z;
    if ( only || isEmpty() )
	return;

    if ( ajustlayers )
	adjustLayers( z );

    prepareUse();
}


void Strat::LayerSequence::setOverburdenVelocity( float vel )
{
    velabove_ = vel;
}


const Strat::RefTree& Strat::LayerSequence::refTree() const
{
    return isEmpty() ? RT() : layers_[0]->refTree();
}


int Strat::LayerSequence::layerIdxAtZ( float zreq ) const
{
    const int nrlays = layers_.size();
    if ( nrlays == 0 || zreq < layers_[0]->zTop()
		     || zreq > layers_[nrlays-1]->zBot() )
	return -1;

    for ( int ilay=0; ilay<nrlays; ilay++ )
    {
	if ( zreq < layers_[ilay]->zBot() )
	    return ilay;
    }
    return nrlays-1;
}


int Strat::LayerSequence::nearestLayerIdxAtZ( float zreq ) const
{
    const int nrlays = layers_.size();
    if ( nrlays < 2 )
	return nrlays == 1 ? 0 : -1;

    for ( int ilay=0; ilay<nrlays; ilay++ )
    {
	if ( zreq < layers_[ilay]->zBot() )
	    return ilay;
    }
    return nrlays - 1;
}


Interval<float> Strat::LayerSequence::zRange() const
{
    if ( isEmpty() )
	return Interval<float>( z0_, z0_ );

    return Interval<float>( z0_, layers_[layers_.size()-1]->zBot() );
}


Interval<float> Strat::LayerSequence::propRange( int propnr ) const
{
    if ( propnr < 0 )
	return zRange();

    Interval<float> rg( mUdf(float), mUdf(float) );
    const int nrlays = layers_.size();
    if ( nrlays < 1 || propnr >= propertyRefs().size() )
	return rg;

    for ( const auto* lay : layers_ )
    {
	const float layval = lay->value( propnr );
	if ( mIsUdf(layval) )
	    continue;

	if ( mIsUdf(rg.start_) )
	    rg.start_ = rg.stop_ = layval;
	else
	    rg.include( layval );
    }

    return rg;
}


int Strat::LayerSequence::indexOf( const Level& lvl, int startat ) const
{
    const RefTree& rt = refTree();
    UnitRefIter it( rt, UnitRefIter::LeavedNodes );
    const LeavedUnitRef* lvlunit = nullptr;
    while ( it.next() )
    {
	const auto* un = static_cast<const LeavedUnitRef*>( it.unit() );
	if ( un->levelID() == lvl.id() )
	{ lvlunit = un; break; }
    }
    if ( !lvlunit ) return -1;

    for ( int ilay=startat; ilay<size(); ilay++ )
    {
	const LeafUnitRef& lur = layers_[ilay]->unitRef();
	if ( lur.upNode() == lvlunit )
	    return ilay;
    }
    return -1;
}


float Strat::LayerSequence::depthOf( const Level& lvl, float notfoundval ) const
{
    const int sz = size();
    if ( sz < 1 )
	return notfoundval;

    const int idx = indexOf( lvl, 0 );
    return idx < 0 ? layers_[sz-1]->zBot() : layers_[idx]->zTop();
}


int Strat::LayerSequence::positionOf( const Level& lvl ) const
{
    const RefTree& rt = refTree();
    UnitRefIter it( rt, UnitRefIter::LeavedNodes );
    ObjectSet<const UnitRef> unlist; BoolTypeSet isabove;
    bool foundlvl = false;
    while ( it.next() )
	// gather all units below level into unlist
    {
	const auto* un = static_cast<const LeavedUnitRef*>( it.unit() );
	if ( un->levelID() == lvl.id() )
	    { foundlvl = true; unlist += un; }
	else if ( foundlvl )
	    unlist += un;
    }
    if ( !foundlvl )
	return -1;

    for ( int ilay=0; ilay<size(); ilay++ )
		// find first layer whose parent is in unlist
    {
	const LeafUnitRef& un = layers_[ilay]->unitRef();
	for ( int iun=0; iun<unlist.size(); iun++ )
	{
	    if ( unlist[iun]->isParentOf(un) )
		return ilay;
	}
    }

    // level must be below last layer
    return size();
}


float Strat::LayerSequence::depthPositionOf( const Level& lvl,
					     float notfoundval ) const
{
    const int sz = size();
    if ( sz < 1 )
	return notfoundval;

    const int idx = positionOf( lvl );
    if ( idx < 0 )
	return notfoundval;

    return idx >= sz ? layers_[sz-1]->zBot() : layers_[idx]->zTop();
}


void Strat::LayerSequence::getLayersFor( const UnitRef* ur,
					 ObjectSet<const Layer>& lys ) const
{
    const int sz = size();
    if ( sz < 1 )
	return;
    if ( !ur )
	ur = &refTree();

    for ( int ilay=0; ilay<sz; ilay++ )
    {
	const Layer* ly = layers_[ilay];
	if ( ur == &ly->unitRef() || ur->isParentOf(ly->unitRef()) )
	    lys += ly;
    }
}


void Strat::LayerSequence::getSequencePart( const Interval<float>& depthrg,
					    bool cropfirstlast,
					    LayerSequence& out ) const
{
    out.setEmpty();
    const int sz = size();
    if ( sz < 1 || depthrg.isUdf() )
	return;

    for ( int ilay=0; ilay<layers_.size(); ilay++ )
    {
	const Layer& lay = *layers_[ilay];
	if ( lay.zBot() < depthrg.start_ + 1e-6f )
	    continue;
	else if ( lay.zTop() > depthrg.stop_ - 1e-6f )
	    break;

	auto* newlay = new Layer( lay );
	if ( lay.zTop() < depthrg.start_ )
	    newlay->setThickness( lay.zBot() - depthrg.start_ );
	else if ( lay.zBot() > depthrg.stop_ )
	    newlay->setThickness( depthrg.stop_ - lay.zTop() );

	out.layers() += newlay;
    }

    out.setStartDepth( depthrg.start_ );
}


void Strat::LayerSequence::adjustLayers( float startz )
{
    if ( startDepth() > startz-1e-2f ||
	 zRange().stop_ < startz-1e-2f )
	return;

    ObjectSet<Layer>& lays = layers();
    while ( !lays.isEmpty() )
    {
	Layer* lay = lays.first();
	const float th = lay->zBot() + startz;
	if ( th < 0.f )
	{
	    delete lays.removeSingle( 0 );
	    continue;
	}

	if ( th > 1e-2f )
	    lay->setThickness( th );

	break;
    }
}


void Strat::LayerSequence::prepareUse() const
{
    float z = z0_;
    for ( int ilay=0; ilay<size(); ilay++ )
    {
	auto& ly = *const_cast<Layer*>( layers_[ilay] );
	ly.setZTop( z );
	z += ly.thickness();
    }
}
