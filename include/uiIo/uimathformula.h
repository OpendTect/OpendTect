#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2014
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "uistrings.h"
#include "mathformula.h"
#include "mnemonics.h"
class uiButton;
class uiUnitSel;
class uiToolButton;
class uiToolButtonSetup;
class uiMathExpression;
class uiMathExpressionVariable;
class CtxtIOObj;
class UnitOfMeasure;
namespace Math { class Form; }


/* edits a Math::Formula */


mExpClass(uiIo) uiMathFormula : public uiGroup
{ mODTextTranslationClass(uiMathFormula);
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup( const uiString& lbl=uiStrings::sEmptyString() )
			    : label_(lbl)
			    , maxnrinps_(6)
			    , withsubinps_(false)
			    , withunits_(true)
			    , proptype_(Mnemonic::Other)	{}

	mDefSetupMemb(uiString,label);
	mDefSetupMemb(int,maxnrinps);
	mDefSetupMemb(BufferString,stortype); // if empty, no I/O
	mDefSetupMemb(bool,withsubinps);
	mDefSetupMemb(bool,withunits);
	mDefSetupMemb(Mnemonic::StdType,proptype); // used if withunits_

    };

			uiMathFormula(uiParent*,Math::Formula&,const Setup&);
			~uiMathFormula();


    void		setNonSpecInputs(const BufferStringSet&,int iinp=-1);
    void		setNonSpecSubInputs(const BufferStringSet&,int iinp=-1);

    bool		setText(const char*);
    const char*		text() const;

    bool		useForm(
			const TypeSet<Mnemonic::StdType>* inptyps=nullptr);
    bool		updateForm() const;

			// shortcuts for things available in form
    int			nrInputs() const;
    const char*		getInput(int) const;
    bool		isSpec(int) const;
    bool		isConst(int) const;
    double		getConstVal(int) const;
    const UnitOfMeasure* getUnit() const;

    uiButton*		addButton(const uiToolButtonSetup&);
    void		addInpViewIcon(const char* icnm,const char* tooltip,
					const CallBack&);

    Notifier<uiMathFormula> formSet;
    Notifier<uiMathFormula> inpSet;
    Notifier<uiMathFormula> subInpSet;
    Notifier<uiMathFormula> formUnitSet;

    uiMathExpression*	exprFld()		{ return exprfld_; }
    int			nrInpFlds() const	{ return inpflds_.size(); }
    uiMathExpressionVariable* inpFld( int idx )	{ return inpflds_[idx]; }
    uiUnitSel*		unitFld()		{ return unitfld_; }
    int			inpSelNotifNr() const	{ return notifinpnr_; }
    int			vwLogInpNr(CallBacker*) const;
    bool		checkValidNrInputs() const;

protected:

    Math::Formula&	form_;
    uiMathExpression*	exprfld_;
    ObjectSet<uiMathExpressionVariable> inpflds_;
    uiUnitSel*		unitfld_;
    uiToolButton*	recbut_;
    uiToolButton*	openbut_;
    uiToolButton*	savebut_;
    int		notifinpnr_;

    Setup		setup_;
    TypeSet<double>	recvals_;
    CtxtIOObj&		ctio_;

    BufferString	getIOFileName(bool forread);
    bool		setNotifInpNr(const CallBacker*);

    void		initFlds(CallBacker*);
    void		formSetCB(CallBacker*);
    void		inpSetCB(CallBacker*);
    void		subInpSetCB(CallBacker*);
    void		formUnitSetCB(CallBacker*);
    void		recButPush(CallBacker*);
    void		readReq(CallBacker*);
    void		writeReq(CallBacker*);

};


