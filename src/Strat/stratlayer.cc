/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
-*/

static const char* rcsID = "$Id: stratlayer.cc,v 1.4 2010-10-06 15:40:52 cvsbert Exp $";

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
	ref->disp_.range_ = Interval<float>( 0, mUdf(float) );
    }

    return *ref;
}


Strat::Layer::Layer( const LeafUnitRef& r, float th )
    : ref_(&r)
    , ztop_(0)
{
    props_.add( new ValueProperty(thicknessRef(),th) );
}


const Strat::LeafUnitRef& Strat::Layer::unitRef() const
{
    return ref_ ? *ref_ : LeafUnitRef::undef();
}


Strat::Layer::ID Strat::Layer::id() const
{
    return unitRef().fullCode();
}


float Strat::Layer::depth() const
{
    const float th = thickness().value();
    float ret = ztop_;
    if ( !mIsUdf(th) )
	ret += 0.5 * th;
    return ret;
}


Strat::RefTree* Strat::LayerModel::gtTree() const
{
    return const_cast<Strat::RefTree*>( isEmpty() ? 0
	    			        : &layers_[0]->unitRef().refTree() );
}


void Strat::LayerModel::getLayersFor( const UnitRef* ur,
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


void Strat::LayerModel::prepareUse()
{
    float z = z0_;
    for ( int idx=0; idx<size(); idx++ )
    {
	Layer& ly = *layers_[idx];
	ly.ztop_ = z;
	z += ly.thickness().value();
    }
}
