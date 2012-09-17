/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.cc,v 1.5 2009/07/22 16:01:45 cvsbert Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visvolren.cc,v 1.5 2009/07/22 16:01:45 cvsbert Exp $";

#include "visvolren.h"
#include <VolumeViz/nodes/SoVolumeRender.h>

mCreateFactoryEntry( visBase::VolrenDisplay );

visBase::VolrenDisplay::VolrenDisplay()
    : visBase::VisualObjectImpl( 0 )
    , volren( new SoVolumeRender )
{
    addChild( volren );
}
