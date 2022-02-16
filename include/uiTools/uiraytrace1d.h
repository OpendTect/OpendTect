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

class uiComboBox;
class uiGenInput;
class uiPushButton;
class uiRayTracerAdvancedDlg;
class uiRayTracerAdvancedGrp;


/*!
\brief Interface for selecting all parameters to run a RayTracer1D
*/

mExpClass(uiTools) uiRayTracer1D : public uiGroup
{ mODTextTranslationClass(uiRayTracer1D);
public:

    mExpClass(uiTools) Setup
    {
	public:
			Setup()
			    : dooffsets_(true)
			    , doreflectivity_(true)
			    , convertedwaves_(true)
			{ withadvanced_ = dooffsets_ && convertedwaves_; }

	mDefSetupMemb(bool,dooffsets);
	mDefSetupMemb(bool,doreflectivity);
	mDefSetupMemb(bool,convertedwaves);
	mDefSetupMemb(bool,withadvanced);
    };

    mDefineFactory2ParamInClass(uiRayTracer1D,uiParent*,const Setup&,factory);

			~uiRayTracer1D();

    virtual uiRetVal	isOK() const;
    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    Notifier<uiRayTracer1D>	parsChanged;

protected:
			uiRayTracer1D(uiParent*,const Setup&);

    bool		doConvertedWaves() const { return convertedwaves_; }
    bool		doReflectivity() const	{ return doreflectivity_; }
    uiGenInput*		lastFld() const		{ return offsetfld_; }

    virtual void	initGrp();
    void		ensureHasAdvancedButton();
    void		setAdvancedGroup(uiRayTracerAdvancedGrp*);
    void		parsChangedCB(CallBacker*);

private:

    void		initGrpCB(CallBacker*);
    void		getAdvancedPush(CallBacker*);

    bool		doreflectivity_;
    bool		convertedwaves_;
    IOPar*		lastiop_ = nullptr;
    uiGenInput*		offsetfld_ = nullptr;
    uiGenInput*		offsetstepfld_ = nullptr;
    uiPushButton*	advbut_ = nullptr;
    uiRayTracerAdvancedGrp* advgrp_ = nullptr;
    uiRayTracerAdvancedDlg* advdlg_ = nullptr;

public:

    virtual uiRayTracerAdvancedGrp* getAvancedGrp(uiParent*);

};


/*!
\brief Basic interface for a uiRayTracer1D
*/

mExpClass(uiTools) uiVrmsRayTracer1D : public uiRayTracer1D
{ mODTextTranslationClass(uiVrmsRayTracer1D);
public:
    mDefaultFactoryInstanciationBase(VrmsRayTracer1D::sFactoryKeyword(),
				     VrmsRayTracer1D::sFactoryDisplayName());

			uiVrmsRayTracer1D(uiParent*,
					const uiRayTracer1D::Setup&);

    static uiRayTracer1D* create( uiParent* p, const uiRayTracer1D::Setup& su )
			    { return new uiVrmsRayTracer1D( p, su ); }

};


/*!
\brief Selector for one or more uiRayTracer1D
*/

mExpClass(uiTools) uiRayTracerSel : public uiGroup
{ mODTextTranslationClass(uiRayTracerSel);
public:
			uiRayTracerSel(uiParent*,const uiRayTracer1D::Setup&);
			~uiRayTracerSel();

    uiRetVal		isOK() const;
    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Notifier<uiRayTracerSel> parsChanged;

private:

    uiComboBox*		raytracerselfld_ = nullptr;
    ObjectSet<uiRayTracer1D> grps_;

    void		setDefault();
    bool		setCurrentType(const char*);

    const uiRayTracer1D* current() const;

    void		initGrpCB(CallBacker*);
    void		selRayTraceCB(CallBacker*);
    void		parsChangedCB(CallBacker*);
};


/*!
\brief Group for the advanced uiRayTracer1D parameters interface
*/

mExpClass(uiTools) uiRayTracerAdvancedGrp : public uiGroup
{ mODTextTranslationClass(uiRayTracerAdvancedGrp);
public:
			uiRayTracerAdvancedGrp(uiParent*,bool convertedwaves,
					       bool doreflectivity);
			~uiRayTracerAdvancedGrp();

    virtual uiRetVal	isOK() const;
    virtual bool	usePar(const IOPar&);
			//<! return false if not changed
    virtual void	fillPar(IOPar&) const;

    Notifier<uiRayTracerAdvancedGrp> parsChanged;

protected:

    void		parsChangedCB(CallBacker*);
    virtual void	initGrp();

    uiGenInput*		lastFld() const		{ return upwavefld_; }

private:

    void		initGrpCB(CallBacker*);

    uiGenInput*		downwavefld_ = nullptr;
    uiGenInput*		upwavefld_ = nullptr;

};
