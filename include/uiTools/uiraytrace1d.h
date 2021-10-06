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

class uiCheckBox;
class uiComboBox;
class uiGenInput;


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
			    , offsetrg_(RayTracer1D::sDefOffsetRange())
			    {}

	mDefSetupMemb(bool,convertedwaves);
	mDefSetupMemb(bool,dooffsets);
	mDefSetupMemb(bool,doreflectivity);
	mDefSetupMemb(bool,showzerooffsetfld);
	mDefSetupMemb(StepInterval<float>,offsetrg);
    };

    mDefineFactory2ParamInClass(uiRayTracer1D,uiParent*,const Setup&,factory);

			~uiRayTracer1D();

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    void		displayOffsetFlds(bool yn);
    bool		isOffsetFldsDisplayed() const;
    void		setOffsetRange(StepInterval<float>);
    bool		doOffsets() const	{ return offsetfld_; }
    bool		hasZeroOffsetFld() const{ return iszerooffsetfld_; }
    bool		isZeroOffset() const;

    Notifier<uiRayTracer1D>	parsChanged;

protected:
			uiRayTracer1D(uiParent*,const Setup&);

    void		parsChangedCB(CallBacker*);
    virtual void	doOffsetChanged()		{}

    bool		doreflectivity_;

    uiGenInput*		downwavefld_ = nullptr;
    uiGenInput*		upwavefld_ = nullptr;

    uiGenInput*		offsetfld_ = nullptr;
    uiGenInput*		offsetstepfld_ = nullptr;
    uiCheckBox*		iszerooffsetfld_ = nullptr;
    uiGenInput*		lastfld_ = nullptr;

private:

    void		initGrp(CallBacker*);
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

    static void		initClass();
};


mExpClass(uiTools) uiRayTracerSel : public uiGroup
{ mODTextTranslationClass(uiRayTracerSel);
public:
			uiRayTracerSel(uiParent*,const uiRayTracer1D::Setup&);
			~uiRayTracerSel();

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    const uiRayTracer1D* current() const;
    uiRayTracer1D*	current();

    bool		setCurrentType(const char*);

    Notifier<uiRayTracerSel> parsChanged;

protected:

    uiComboBox*		raytracerselfld_ = nullptr;
    ObjectSet<uiRayTracer1D> grps_;

    bool		setCurrent(int);

    void		initGrp(CallBacker*);
    void		selRayTraceCB(CallBacker*);
    void		parsChangedCB(CallBacker*);
};


