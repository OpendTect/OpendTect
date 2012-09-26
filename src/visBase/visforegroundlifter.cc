/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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


SoNode* ForegroundLifter::gtInvntrNode()
{ return lifter_; }

}; // namespace visBase
