/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.cc,v 1.6 2012-05-02 11:54:11 cvskris Exp $
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: visvolren.cc,v 1.6 2012-05-02 11:54:11 cvskris Exp $";

#include "visvolren.h"
#include <VolumeViz/nodes/SoVolumeRender.h>

mCreateFactoryEntry( visBase::VolrenDisplay );

visBase::VolrenDisplay::VolrenDisplay()
    : visBase::VisualObjectImpl( 0 )
    , volren( new SoVolumeRender )
{
    addChild( volren );
}
