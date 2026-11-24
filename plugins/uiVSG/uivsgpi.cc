/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________


-*/

#include "uivsgmod.h"

#include "moddepmgr.h"
#include "thread.h"
#include "visvsgdata.h"


mDefModInitFn(uiVSG)
{
    mIfNotFirstTime( return );
    ODVsg::DataObject::setVisualizationThread( Threads::currentThread() );
}
