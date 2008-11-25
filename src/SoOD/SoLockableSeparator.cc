/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoLockableSeparator.cc,v 1.2 2008-11-25 15:35:22 cvsbert Exp $";


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



