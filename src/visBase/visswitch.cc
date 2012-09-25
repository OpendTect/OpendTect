/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : Sep 2012
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "visswitch.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"


namespace visBase
{
mCreateFactoryEntry( Switch );
mCreateFactoryEntry( Separator );


Switch::Switch()
    : switch_( new SoSwitch )
{
    switch_->ref();
}


Switch::~Switch()
{ switch_->unref(); }


SoNode* Switch::gtInvntrNode()
{ return switch_; }


void Switch::turnOn( bool yn )
{ switch_->whichChild.setValue( yn ? -3 : -1 ); }


void Switch::addChild( SoNode* sn )
{ switch_->addChild( sn ); }


void Switch::removeChild( SoNode* sn )
{ switch_->removeChild( sn ); }


Separator::Separator()
    : separator_( new SoSeparator )
{
    separator_->ref();
}


Separator::~Separator()
{ separator_->unref(); }


SoNode* Separator::gtInvntrNode()
{ return separator_; }


void Separator::addChild( SoNode* sn )
{ separator_->addChild( sn ); }


void Separator::removeChild( SoNode* sn )
{ separator_->removeChild( sn ); }


}; //namespace
