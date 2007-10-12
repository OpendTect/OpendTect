/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.cc,v 1.3 2007-10-12 19:14:34 cvskris Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visvolren.cc,v 1.3 2007-10-12 19:14:34 cvskris Exp $";

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
