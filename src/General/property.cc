/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: property.cc,v 1.1 2004-01-05 14:48:53 bert Exp $";

#include "propertyimpl.h"
#include "mathexpression.h"
#include "errh.h"


DefineEnumNames(PropertyRef,StdType,0,"Standard Property")
{
	"None (User-defined)",
	"P-wave sonic travel time",
	"Density",
	"Acoustic Impedance",
	"Porosity",
	"Permeability",
	"Water saturation",
	"S-wave sonic travel time",
	"P-wave velocity",
	"S-wave velocity",
	"Gamma Ray",
	"Spontaneous Potential",
	"Poisson's Ratio",
	"Resistivity",
	"Compressibility",
	"Caliper",
	"Temperature",
	"Pressure",
	0
};


MathProperty::~MathProperty()
{
    delete expr_;
}


void MathProperty::setDef( const char* s )
{
    inps_.erase();
    def_ = s;
    delete expr_; expr_ = MathExpression::parse( def_ );
    if ( !expr_ ) return;
    const int sz = expr_->getNrVariables();
    while ( sz > inps_.size() )
	inps_ += 0;
}


int MathProperty::nrInputs() const
{
    return expr_ ? expr_->getNrVariables() : 0;
}


const char* MathProperty::inputName( int idx ) const
{
    return expr_ ? expr_->getVariableStr(idx) : 0;
}


void MathProperty::setInput( int idx, const Property* p )
{
    if ( p && p->dependsOn(this) )
    {
	BufferString msg( "Invalid cyclic dependency for property " );
	msg += ref()->name();
	ErrMsg( msg );
	p = 0;
    }
    inps_.replace( p, idx );
}


bool MathProperty::dependsOn( const Property* p ) const
{
    if ( p == this ) return true;
    else if ( !p ) return false;

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* inp = inps_[idx];
	if ( inp && inp->dependsOn(p) )
	    return true;
    }
    return false;
}


float MathProperty::value() const
{
    if ( !expr_ ) return mUndefValue;

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* p = inps_[idx];
	if ( !p ) return mUndefValue;
	expr_->setVariable( idx, inps_[idx]->value() );
    }

    return expr_->getValue();
}
