/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.cc,v 1.7 2012-05-02 15:12:35 cvskris Exp $
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: visvolren.cc,v 1.7 2012-05-02 15:12:35 cvskris Exp $";

#include "visvolren.h"
#include <VolumeViz/nodes/SoVolumeRender.h>

mCreateFactoryEntry( visBase::VolrenDisplay );

visBase::VolrenDisplay::VolrenDisplay()
    : visBase::VisualObjectImpl( 0 )
    , volren( new SoVolumeRender )
{
    addChild( volren );
}
