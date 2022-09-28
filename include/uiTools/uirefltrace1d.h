#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "reflcalc1d.h"
#include "uigroup.h"
#include "uistring.h"

class uiComboBox;
class uiGenInput;
class uiPushButton;
class uiReflCalcAdvancedDlg;
class uiReflCalcAdvancedGrp;


/*!
\brief Interface for selecting all parameters to run a ReflCalc1D
*/

mExpClass(uiTools) uiReflCalc1D : public uiGroup
{ mODTextTranslationClass(uiReflCalc1D);
public:

    mExpClass(uiTools) Setup
    {
	public:
			Setup(bool singleangle);
	virtual		~Setup();

	mDefSetupMemb(bool,singleangle);
	mDefSetupMemb(bool,doangles);		// def: true
	mDefSetupMemb(bool,withadvanced);	// def: true
    };

    mDefineFactory2ParamInClass(uiReflCalc1D,uiParent*,const Setup&,factory);

			~uiReflCalc1D();

    virtual uiRetVal	isOK() const;
    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    Notifier<uiReflCalc1D>	parsChanged;

protected:
			uiReflCalc1D(uiParent*,const Setup&);

    static bool		doReflectivity()	{ return true; }
    uiGenInput*		lastFld() const		{ return anglefld_; }

    virtual void	initGrp();
    void		ensureHasAdvancedButton();
    void		setAdvancedGroup(uiReflCalcAdvancedGrp*);
    void		parsChangedCB(CallBacker*);

private:

    void		initGrpCB(CallBacker*);
    void		getAdvancedPush(CallBacker*);

    IOPar*		lastiop_ = nullptr;
    uiGenInput*		anglefld_ = nullptr;
    uiGenInput*		anglestepfld_ = nullptr;
    uiPushButton*	advbut_ = nullptr;
    uiReflCalcAdvancedGrp* advgrp_ = nullptr;
    uiReflCalcAdvancedDlg* advdlg_ = nullptr;

public:

    virtual uiReflCalcAdvancedGrp* getAvancedGrp(uiParent*);

};


/*!
\brief Basic interface for a uiReflCalc1D
*/

mExpClass(uiTools) uiAICalc1D : public uiReflCalc1D
{ mODTextTranslationClass(uiAICalc1D);
public:
    mDefaultFactoryInstanciationBase(AICalc1D::sFactoryKeyword(),
				     AICalc1D::sFactoryDisplayName());

			uiAICalc1D(uiParent*,const uiReflCalc1D::Setup&);
			~uiAICalc1D();

    bool		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;

    static uiReflCalc1D* create( uiParent* p, const uiReflCalc1D::Setup& su )
			    { return new uiAICalc1D( p, su ); }

};


/*!
\brief Selector for one or more uiRelfCalc1D
*/

mExpClass(uiTools) uiReflCalcSel : public uiGroup
{ mODTextTranslationClass(uiReflCalcSel);
public:
			uiReflCalcSel(uiParent*,const uiReflCalc1D::Setup&);
			~uiReflCalcSel();

    uiRetVal		isOK() const;
    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Notifier<uiReflCalcSel> parsChanged;

private:

    uiComboBox*		reflcalcselfld_ = nullptr;
    ObjectSet<uiReflCalc1D> grps_;

    void		setDefault();
    bool		setCurrentType(const char*);

    const uiReflCalc1D* current() const;

    void		initGrpCB(CallBacker*);
    void		selReflCalcCB(CallBacker*);
    void		parsChangedCB(CallBacker*);
};


/*!
\brief Group for the advanced uiReflCalc1D parameters interface
*/

mExpClass(uiTools) uiReflCalcAdvancedGrp : public uiGroup
{ mODTextTranslationClass(uiReflCalcAdvancedGrp);
public:
			uiReflCalcAdvancedGrp(uiParent*);
			~uiReflCalcAdvancedGrp();

    virtual uiRetVal	isOK() const;
    virtual bool	usePar(const IOPar&);
			//<! return false if not changed
    virtual void	fillPar(IOPar&) const;

    Notifier<uiReflCalcAdvancedGrp> parsChanged;

protected:

    void		parsChangedCB(CallBacker*);
    virtual void	initGrp();

    uiGenInput*		lastFld() const		{ return nullptr; }

private:

    void		initGrpCB(CallBacker*);

};
