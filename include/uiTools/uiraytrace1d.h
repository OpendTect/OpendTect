#ifndef uiraytracer_h
#define uiraytracer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: uiraytrace1d.h,v 1.6 2011-10-07 12:14:15 cvsbruno Exp $
________________________________________________________________________


-*/

#include "raytrace1d.h"
#include "uigroup.h"

class uiGenInput;
class uiLabeledComboBox;


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
			    , offsetrg_(0,3000,100)
			    , raysetup_(rsu)
			    {}

	mDefSetupMemb(bool,convertedwaves);
	mDefSetupMemb(bool,dosourcereceiverdepth);
	mDefSetupMemb(bool,dooffsets);
	mDefSetupMemb(StepInterval<float>,offsetrg);
	mDefSetupMemb(const RayTracer1D::Setup*,raysetup);
    };

    mDefineFactory2ParamInClass(uiRayTracer1D,uiParent*,const Setup&,factory);

    bool		fill(RayTracer1D::Setup&);
    virtual void	fillPar(IOPar& par) const;

    void 		getOffsets(StepInterval<float>&) const;
    void		displayOffsetFlds(bool yn);

protected:
			uiRayTracer1D(uiParent*,const Setup&);

    uiGenInput*		srcdepthfld_;
    uiGenInput*		downwavefld_;
    uiGenInput*		upwavefld_;

    uiGenInput* 	offsetfld_;
    uiGenInput* 	offsetstepfld_;

    uiRayTracer1D::Setup          setup_;
};


mClass uiRayTracerSel : public uiGroup
{
public:
    			uiRayTracerSel(uiParent*,const uiRayTracer1D::Setup&);
protected:

    uiLabeledComboBox*	raytracerselbox_;

    ObjectSet<uiRayTracer1D> grps_;

    void		selRayTraceCB(CallBacker*);
};


mClass uiVrmsRayTracer1D : public uiRayTracer1D
{
public:
			uiVrmsRayTracer1D(uiParent*,
					const uiRayTracer1D::Setup&);

    static uiRayTracer1D* create(uiParent* p,const uiRayTracer1D::Setup& s)
			    { return new uiVrmsRayTracer1D(p,s); }
    static void         initClass();
};


#endif
