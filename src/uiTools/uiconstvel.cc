/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiconstvel.h"

#include "uistring.h"
#include "unitofmeasure.h"

namespace Vel
{

float getGUIDefaultVelocity()
{
    return SI().depthsInFeet() ? 8000.f : 2000.f;
}

} // namespace Vel



uiConstantVel::uiConstantVel( uiParent* p, float defvel, const uiString& lbl )
    : uiGenInput(p,tr( "%1 %2" )
		   .arg(lbl.isSet() ? lbl : uiStrings::sVelocity())
		   .arg(UnitOfMeasure::surveyDefVelUnitAnnot(true,true)),
		   FloatInpSpec(defvel) )
{}


uiConstantVel::~uiConstantVel()
{}


void uiConstantVel::setInternalVelocity( float vel )
{
    this->setValue(
	     getConvertedValue( vel, 0, UnitOfMeasure::surveyDefDepthUnit() ) );
}


float uiConstantVel::getInternalVelocity() const
{
    return getConvertedValue( this->getFValue(),
			      UnitOfMeasure::surveyDefDepthUnit(), 0 );
}
