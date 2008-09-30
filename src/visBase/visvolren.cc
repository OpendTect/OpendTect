/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.cc,v 1.4 2008-09-30 08:31:46 cvsbert Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visvolren.cc,v 1.4 2008-09-30 08:31:46 cvsbert Exp $";

#include "visvolren.h"
#include <VolumeViz/nodes/SoVolumeRender.h>

mCreateFactoryEntry( visBase::VolrenDisplay );

visBase::VolrenDisplay::VolrenDisplay()
    : visBase::VisualObjectImpl( 0 )
    , volren( new SoVolumeRender )
{
    addChild( volren );
}
