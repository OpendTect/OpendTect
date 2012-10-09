/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "SoLockableSeparator.h"


SO_NODE_SOURCE( SoLockableSeparator );

void SoLockableSeparator::initClass()
{
    SO_NODE_INIT_CLASS(SoLockableSeparator, SoSeparator, "Separator");
}


SoLockableSeparator::SoLockableSeparator()
    : lock( SbRWMutex::WRITE_PRECEDENCE )
{
    SO_NODE_CONSTRUCTOR( SoLockableSeparator );
}



