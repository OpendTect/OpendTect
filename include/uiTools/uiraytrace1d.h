#ifndef uiraytracer_h
#define uiraytracer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uitoolsmod.h"
#include "raytrace1d.h"
#include "uigroup.h"

class uiGenInput;
class uiCheckBox;
class uiLabeledComboBox;


mExpClass(uiTools) uiRayTracer1D : public uiGroup
{
public:

    mExpClass(uiTools) Setup 		
    {
	public:	
			Setup()
			    : convertedwaves_(false)
			    , doreflectivity_(true)
			    , dooffsets_(false)
			    , offsetrg_(0,6000,100)
			    {}

	mDefSetupMemb(bool,convertedwaves);
	mDefSetupMemb(bool,dooffsets);
	mDefSetupMemb(bool,doreflectivity);
	mDefSetupMemb(StepInterval<float>,offsetrg);
    };

    mDefineFactory2ParamInClass(uiRayTracer1D,uiParent*,const Setup&,factory);

    virtual bool 	usePar(const IOPar&);
    virtual void  	fillPar(IOPar&) const;

    void		displayOffsetFlds(bool yn); 
    void		setOffsetRange(StepInterval<float>);
    bool		doOffsets() const	{ return offsetfld_; }
    Notifier<uiGenInput>& offsetChanged();

protected:
			uiRayTracer1D(uiParent*,const Setup&);

    bool 		doreflectivity_;

    uiGenInput*		downwavefld_;
    uiGenInput*		upwavefld_;

    uiGenInput* 	offsetfld_;
    uiGenInput* 	offsetstepfld_;
    uiGenInput*		lastfld_;
};


mExpClass(uiTools) uiVrmsRayTracer1D : public uiRayTracer1D
{
public:
			uiVrmsRayTracer1D(uiParent*,
					const uiRayTracer1D::Setup&);

    static uiRayTracer1D* create(uiParent* p,const uiRayTracer1D::Setup& s)
			    { return new uiVrmsRayTracer1D(p,s); }

    static void         initClass();
};


mExpClass(uiTools) uiRayTracerSel : public uiGroup
{
public:
    			uiRayTracerSel(uiParent*,const uiRayTracer1D::Setup&);

    void                usePar(const IOPar&);
    void                fillPar(IOPar&) const;

    uiRayTracer1D*	current();
    const uiRayTracer1D* current() const;
    Notifier<uiRayTracerSel> offsetChanged;

protected:

    uiLabeledComboBox*	raytracerselfld_;

    ObjectSet<uiRayTracer1D> grps_;

    void		selRayTraceCB(CallBacker*);
    void		offsChangedCB(CallBacker*);
};


#endif

