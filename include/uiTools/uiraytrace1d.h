#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
________________________________________________________________________


-*/

#include "uitoolsmod.h"
#include "raytrace1d.h"
#include "uigroup.h"
#include "uistring.h"

class uiGenInput;
class uiCheckBox;
class uiLabeledComboBox;


mExpClass(uiTools) uiRayTracer1D : public uiGroup
{ mODTextTranslationClass(uiRayTracer1D);
public:

    mExpClass(uiTools) Setup
    {
	public:
			Setup()
			    : convertedwaves_(false)
			    , doreflectivity_(true)
			    , dooffsets_(false)
			    , showzerooffsetfld_(true)
			    , offsetrg_(0,6000,100)
			    {}

	mDefSetupMemb(bool,convertedwaves);
	mDefSetupMemb(bool,dooffsets);
	mDefSetupMemb(bool,doreflectivity);
	mDefSetupMemb(bool,showzerooffsetfld);
	mDefSetupMemb(StepInterval<float>,offsetrg);
    };

    mDefineFactory2ParamInClass(uiRayTracer1D,uiParent*,const Setup&,factory);

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    void		setOffsetRange(const StepInterval<float>&);
    bool		doOffsets() const	{ return offsetfld_; }
    bool		hasZeroOffsetFld() const{ return iszerooffsetfld_; }
    bool		isZeroOffset() const;

    Notifier<uiRayTracer1D>	offsetChanged;

protected:
			uiRayTracer1D(uiParent*,const Setup&);

    bool		doreflectivity_;

    uiGenInput*		downwavefld_;
    uiGenInput*		upwavefld_;
    uiGenInput*	offsetfld_;
    uiCheckBox*		iszerooffsetfld_;

    uiGenInput*		lastfld_;

    void		zeroOffsetChecked(CallBacker*);
    void		offsetChangedCB(CallBacker*);
};


mExpClass(uiTools) uiVrmsRayTracer1D : public uiRayTracer1D
{ mODTextTranslationClass(uiVrmsRayTracer1D);
public:
			uiVrmsRayTracer1D(uiParent*,
					const uiRayTracer1D::Setup&);

    static uiRayTracer1D* create(uiParent* p,const uiRayTracer1D::Setup& s)
			    { return new uiVrmsRayTracer1D(p,s); }

    static void         initClass();
};


mExpClass(uiTools) uiRayTracerSel : public uiGroup
{ mODTextTranslationClass(uiRayTracerSel);
public:
			uiRayTracerSel(uiParent*,const uiRayTracer1D::Setup&);

    void                usePar(const IOPar&);
    void                fillPar(IOPar&) const;

    uiRayTracer1D*	current();
    const uiRayTracer1D* current() const;
    bool		setCurrent(int);
    bool		setCurrentType(const char*);
    Notifier<uiRayTracerSel> offsetChanged;

protected:

    uiLabeledComboBox*	raytracerselfld_;

    ObjectSet<uiRayTracer1D> grps_;

    void		selRayTraceCB(CallBacker*);
    void		offsChangedCB(CallBacker*);
};
