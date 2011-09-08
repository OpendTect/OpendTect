#ifndef uiraytracer_h
#define uiraytracer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: uiraytrace1d.h,v 1.4 2011-09-08 14:16:05 cvsbruno Exp $
________________________________________________________________________


-*/

#include "raytrace1d.h"
#include "uigroup.h"


class uiGenInput;


mClass uiRayTracer1D : public uiGroup
{
public:

    mClass Setup 		
    {
	public:	
			Setup(const RayTracer1D::Setup* rsu=0)
			    : convertedwaves_(false)
			    , dosourcereceiverdepth_(true)
			    , dooffsets_(false)
			    , offsetrg_(0,RayTracer1D::sKeyStdMaxOffset(),
					  RayTracer1D::sKeyStdStep())
			    , raysetup_(rsu)
			    {}

	mDefSetupMemb(bool,convertedwaves);
	mDefSetupMemb(bool,dosourcereceiverdepth);
	mDefSetupMemb(bool,dooffsets);
	mDefSetupMemb(StepInterval<float>,offsetrg);
	mDefSetupMemb(const RayTracer1D::Setup*,raysetup);
    };

			uiRayTracer1D(uiParent*,const Setup&);

    bool		fill(RayTracer1D::Setup&);

    void 		getOffsets(StepInterval<float>&) const;
    void		displayOffsetFlds(bool yn);

protected:

    uiGenInput*		srcdepthfld_;
    uiGenInput*		downwavefld_;
    uiGenInput*		upwavefld_;

    uiGenInput* 	offsetfld_;
    uiGenInput* 	offsetstepfld_;
};





#endif
