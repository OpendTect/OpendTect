/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : Marc Gerritsen
 * DATE     : 25-6-2003
-*/

static const char* rcsID = "$Id: visdragger.cc,v 1.1 2003-07-08 09:54:40 jeroen Exp $";

#include "visdragger.h"

#include <Inventor/draggers/SoDragger.h>

visBase::Dragger::Dragger( SoDragger* dragger_ )
    : dragger( dragger_ )
    , starthappened( this )
    , stophappened( this )
    , movehappened( this )
{
    dragger->addStartCallback( startCB, this);
    dragger->addFinishCallback( stopCB, this);
    dragger->addMotionCallback( moveCB, this);
}


visBase::Dragger::~Dragger()
{}


void visBase::Dragger::startCB( void* ptr, SoDragger* )
{
    Dragger* thisp = ( Dragger* ) ptr;
    thisp->starthappened.trigger( thisp );
}


void visBase::Dragger::moveCB( void* ptr, SoDragger* )
{
    Dragger* thisp = ( Dragger* ) ptr;
    thisp->movehappened.trigger( thisp );
}


void visBase::Dragger::stopCB( void* ptr, SoDragger* )
{
    Dragger* thisp = ( Dragger* ) ptr;
    thisp->stophappened.trigger( thisp );
}

