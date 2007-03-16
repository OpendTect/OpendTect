#ifndef visvolrendisplay_h
#define visvolrendisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.h,v 1.2 2007-03-16 11:30:51 cvsnanne Exp $
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

    static visBase::FactoryEntry oldnameentry;
};

}; //Namespace

#endif
