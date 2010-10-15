/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/

static const char* rcsID = "$Id: stratlayer.cc,v 1.6 2010-10-15 13:38:41 cvsbert Exp $";

#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratreftree.h"
#include "propertyimpl.h"
#include "propertyref.h"


const PropertyRef& Strat::Layer::thicknessRef()
{
    PropertyRef* ref = 0;
    if ( !ref )
    {
	ref = new PropertyRef( "Thickness", PropertyRef::Dist );
	ref->aliases().add( "thick" );
	const PropertyRef* thref = PROPS().find( "thickness" );
	if ( thref )
	    ref->disp_ = thref->disp_;
	else
	    ref->disp_.range_ = Interval<float>( 0, 100 );
    }

    return *ref;
}


Strat::Layer::Layer( const LeafUnitRef& r )
    : ref_(&r)
{
    setValue( 0, 0 ); setValue( 1, 0 );
}


const Strat::LeafUnitRef& Strat::Layer::unitRef() const
{
    return ref_ ? *ref_ : LeafUnitRef::undef();
}


Strat::Layer::ID Strat::Layer::id() const
{
    return unitRef().fullCode();
}


float Strat::Layer::value( int ival ) const
{
    return ival < vals_.size() ? vals_[ival] : mUdf(float);
}


void Strat::Layer::setValue( int ival, float val )
{
    while ( vals_.size() <= ival )
	vals_ += mUdf(float);
    vals_[ival] = val;
}


Strat::LayerSequence::LayerSequence( const PropertyRefSelection* prs )
    : z0_(0)
{
    if ( prs ) props_ = *prs;
}


Strat::LayerSequence::~LayerSequence()
{
    deepErase( layers_ );
}


Strat::LayerSequence& Strat::LayerSequence::operator =(
					const Strat::LayerSequence& oth )
{
    if ( this != &oth )
    {
	deepCopy( layers_, oth.layers_ );
	z0_ = oth.z0_;
	props_ = oth.props_;
    }
    return *this;
}


const Strat::RefTree* Strat::LayerSequence::refTree() const
{
    return isEmpty() ? 0 : &layers_[0]->unitRef().refTree();
}


void Strat::LayerSequence::getLayersFor( const UnitRef* ur,
					 ObjectSet<const Layer>& lys ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    if ( !ur ) ur = refTree();

    for ( int idx=0; idx<sz; idx++ )
    {
	const Layer* ly = layers_[idx];
	if ( ur == &ly->unitRef() || ur->isParentOf(ly->unitRef()) )
	    lys += ly;
    }
}


void Strat::LayerSequence::prepareUse()
{
    float z = z0_;
    for ( int idx=0; idx<size(); idx++ )
    {
	Layer& ly = *layers_[idx];
	ly.setZTop( z );
	z += ly.thickness();
    }
}


Strat::LayerModel::LayerModel()
{
    props_ += &Layer::thicknessRef();
}


Strat::LayerModel::~LayerModel()
{
    deepErase( seqs_ );
}


Strat::LayerModel& Strat::LayerModel::operator =( const Strat::LayerModel& oth )
{
    if ( this != &oth )
    {
	for ( int iseq=0; iseq<oth.seqs_.size(); iseq++ )
	{
	    LayerSequence* newseq = new LayerSequence( *oth.seqs_[iseq] );
	    newseq->setPropertyRefs( props_ );
	    seqs_ += newseq;
	}
	props_ = oth.props_;
    }
    return *this;
}


Strat::LayerSequence& Strat::LayerModel::addSequence()
{
    LayerSequence* newseq = new LayerSequence( &props_ );
    seqs_ += newseq;
    return *newseq;
}


void Strat::LayerModel::setEmpty()
{
    deepErase( seqs_ );
}


const Strat::RefTree* Strat::LayerModel::refTree() const
{
    return isEmpty() ? 0 : seqs_[0]->refTree();
}
