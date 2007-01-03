/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.cc,v 1.1 2007-01-03 18:24:26 cvskris Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visvolren.cc,v 1.1 2007-01-03 18:24:26 cvskris Exp $";

#include "visvolren.h"

#ifdef YES
#undef YES
#endif

#ifdef NO
#undef NO
#endif

#include <VolumeViz/nodes/SoVolumeRender.h>

mCreateFactoryEntry( visBase::VolrenDisplay );

visBase::VolrenDisplay::VolrenDisplay()
    : visBase::VisualObjectImpl( 0 )
    , volren( new SoVolumeRender )
{
    addChild( volren );
}
