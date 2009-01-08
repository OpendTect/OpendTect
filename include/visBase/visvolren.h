#ifndef visvolrendisplay_h
#define visvolrendisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.h,v 1.4 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________

-*/


#include "visobject.h"

class SoROI;
class SoVolumeRender;

namespace visBase
{

mClass VolrenDisplay : public visBase::VisualObjectImpl
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
