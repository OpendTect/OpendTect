#ifndef visvolrendisplay_h
#define visvolrendisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.h,v 1.3 2007-10-12 19:14:34 cvskris Exp $
________________________________________________________________________

-*/


#include "visobject.h"

class SoROI;
class SoVolumeRender;

namespace visBase
{

class VolrenDisplay : public visBase::VisualObjectImpl
{
public:
    static VolrenDisplay*	create()
				mCreateDataObj(VolrenDisplay);

protected:
    SoROI*			roi;
    SoVolumeRender*		volren;
};

}; //Namespace

#endif
