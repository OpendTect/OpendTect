#ifndef visvolren_h
#define visvolren_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2004
 RCS:           $Id: visvolren.h,v 1.7 2012-08-03 13:01:27 cvskris Exp $
________________________________________________________________________

-*/


#include "visbasemod.h"
#include "visobject.h"

class SoROI;
class SoVolumeRender;

namespace visBase
{

mClass(visBase) VolrenDisplay : public visBase::VisualObjectImpl
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

