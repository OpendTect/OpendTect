/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.cc,v 1.2 2007-03-16 11:30:51 cvsnanne Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visvolren.cc,v 1.2 2007-03-16 11:30:51 cvsnanne Exp $";

#include "visvolren.h"

#ifdef YES
#undef YES
#endif

#ifdef NO
#undef NO
#endif

#include <VolumeViz/nodes/SoVolumeRender.h>

visBase::FactoryEntry visBase::VolrenDisplay::oldnameentry(
			(visBase::FactPtr) visBase::VolrenDisplay::create,
			"VolumeRender::VolrenDisplay");

mCreateFactoryEntry( visBase::VolrenDisplay );

visBase::VolrenDisplay::VolrenDisplay()
    : visBase::VisualObjectImpl( 0 )
    , volren( new SoVolumeRender )
{
    addChild( volren );
}
