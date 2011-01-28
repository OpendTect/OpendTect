#ifndef uiraytracer_h
#define uiraytracer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: uiraytrace1d.h,v 1.1 2011-01-28 04:49:58 cvskris Exp $
________________________________________________________________________


-*/

#include "raytrace1d.h"
#include "uigroup.h"


class uiGenInput;


mClass uiRayTracer1D : public uiGroup
{
public:

			uiRayTracer1D(uiParent*,bool dosourcereceiverdepth,
				bool convertedwaves,
				const RayTracer1D::Setup* = 0 );
    bool		fill(RayTracer1D::Setup&) const;

protected:

    uiGenInput*		srcdepthfld_;
    uiGenInput*		downwavefld_;
    uiGenInput*		upwavefld_;
};

#endif
