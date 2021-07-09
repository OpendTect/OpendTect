/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/


#include "propertyref.h"
#include "mathproperty.h"
#include "mnemonics.h"
#include "unitofmeasure.h"
#include "survinfo.h"
#include "ascstream.h"
#include "safefileio.h"
#include "ioman.h"
#include "separstr.h"
#include "globexpr.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioman.h"
#include <typeinfo>

mDefineEnumUtils(PropertyRef,StdType,"Standard Property")
{
	"Anisotropy",
	"Area",
	"Classification",
	"Compressibility",
	"Density",
	"Distance/Depth",
	"Elastic Ratio",
	"Electrical Potential",
	"Gamma Ray",
	"Impedance",
	"Permeability",
	"Pressure",
	"Pressure Gradient",
	"Pressure-Weight",
	"Resistivity",
	"Sonic travel time",
	"Temperature",
	"Time",
	"Velocity",
	"Volumetrics", // ratios: relative
	"Volume", // absolute
	"Other",
	0
};


const PropertyRef& PropertyRef::undef()
{
    mDefineStaticLocalObject( PtrMan<PropertyRef>, udf, = 0 );
    if ( !udf )
    {
	PropertyRef* newudf = new PropertyRef( "Undef" );

	udf.setIfNull(newudf,true);
    }
    return *udf;
}


PropertyRef::~PropertyRef()
{}


PropertyRef& PropertyRef::operator =( const PropertyRef& pr )
{
    if ( this != &pr )
    {
	setName( pr.name() );
	stdtype_ = pr.stdtype_;
    }
    return *this;
}


PropertyRef::StdType PropertyRef::surveyZType()
{
    return SI().zIsTime() ? Time : Dist;
}


bool PropertyRef::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm )
	return this == &undef();

    if ( caseInsensitiveEqual(nm,name().buf(),0) )
	return true;

    return false;
}


