/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2006
 RCS:           $Id: SoLockableSeparator.cc,v 1.1 2008-10-30 13:00:40 cvskris Exp $
________________________________________________________________________

-*/


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



