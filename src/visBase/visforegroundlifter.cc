/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visforegroundlifter.cc,v 1.2 2008-11-25 15:35:27 cvsbert Exp $";

#include "visforegroundlifter.h"

#include "SoForegroundTranslation.h"

mCreateFactoryEntry( visBase::ForegroundLifter );

namespace visBase
{

ForegroundLifter::ForegroundLifter()
    : lifter_( new SoForegroundTranslation )
{ lifter_->ref(); }


ForegroundLifter::~ForegroundLifter()
{ lifter_->unref(); }


void ForegroundLifter::setLift( float nl )
{
    lifter_->lift.setValue( nl );
}


float ForegroundLifter::getLift() const
{ return lifter_->lift.getValue(); }


SoNode* ForegroundLifter::getInventorNode()
{ return lifter_; }

}; // namespace visBase
