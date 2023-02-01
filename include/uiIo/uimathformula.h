#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uigroup.h"
#include "mnemonics.h"

class uiButton;
class uiMathExpression;
class uiMathExpressionVariable;
class uiMnemonicsSel;
class uiToolButton;
class uiToolButtonSetup;
class uiUnitSel;

class UnitOfMeasure;
namespace Math { class Formula; }


/* edits a Math::Formula */


mExpClass(uiIo) uiMathFormula : public uiGroup
{ mODTextTranslationClass(uiMathFormula);
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup( const uiString& lbl=uiString::empty() )
			    : label_(lbl)
			    , maxnrinps_(6)
			    , withsubinps_(false)
			    , withunits_(true)
			    , mn_(nullptr)	{}
	virtual		~Setup()		{}

	mDefSetupMemb(uiString,label);
	mDefSetupMemb(int,maxnrinps);
	mDefSetupMemb(BufferString,stortype); // if empty, no I/O
	mDefSetupMemb(bool,withsubinps);
	mDefSetupMemb(bool,withunits);
	mDefSetupMemb(const Mnemonic*,mn); // used if withunits_

    };

			uiMathFormula(uiParent*,Math::Formula&,const Setup&);
			~uiMathFormula();

    bool		setText(const char*);
    bool		useForm();
    void		setNonSpecInputs(const BufferStringSet&,int iinp=-1,
					 const MnemonicSelection* =nullptr);
    void		setNonSpecSubInputs(const BufferStringSet&,int iinp=-1);
    mDeprecatedObs
    void		setFixedFormUnits( bool yn )	{ fixedunits_ = yn; }

    const char*		text() const;
    bool		updateForm() const;

			// shortcuts for things available in form
    int			nrInputs() const;
    const char*		getInput(int) const;
    bool		isSpec(int) const;
    bool		isConst(int) const;
    double		getConstVal(int) const;

    uiButton*		addButton(const uiToolButtonSetup&);
    void		addInpViewIcon(const char* icnm,const char* tooltip,
					const CallBack&);

    Notifier<uiMathFormula> inpSet;
    Notifier<uiMathFormula> subInpSet;
    Notifier<uiMathFormula> formMnSet;
    Notifier<uiMathFormula> formUnitSet;
    CNotifier<uiMathFormula,const Mnemonic*>& formCMnSet();

    uiMathExpression*	exprFld()		{ return exprfld_; }
    int			nrInpFlds() const	{ return inpflds_.size(); }
    uiMathExpressionVariable* inpFld( int idx )	{ return inpflds_[idx]; }
    int			vwLogInpNr(CallBacker*) const;
    bool		checkValidNrInputs() const;

private:
			mOD_DisableCopy(uiMathFormula);

    Math::Formula&	form_;
    uiMathExpression*	exprfld_;
    MnemonicSelection*	mnsel_ = nullptr;
    ObjectSet<uiMathExpressionVariable> inpflds_;
    uiMnemonicsSel*	mnselfld_ = nullptr;
    uiUnitSel*		unitfld_ = nullptr;
    bool		fixedunits_ = false;
    uiToolButton*	recbut_ = nullptr;

    Setup		setup_;
    TypeSet<double>	recvals_;

    Mnemonic::StdType	getOutputStdType() const;
    const uiMnemonicsSel* getMnSelFld() const;
    const uiUnitSel*	getUnitSelFld() const;
    void		setFormMnemonic(const Mnemonic&,bool dispyn);
    uiMnemonicsSel*	getMnSelFld();
    uiUnitSel*		getUnitSelFld();

    BufferString	getIOFileName(bool forread);
    bool		setNotifInpNr(const CallBacker*,int& inpnr);
    void		addFormOutputsDefs();
    bool		guessFormula(Math::Formula&);
    void		guessInputFormDefs();
    bool		guessOutputFormDefs();
    mDeprecatedObs
    bool		setOutputDefsFromForm();
    bool		setOutputDefsFromForm(bool hasfixedunits);
    bool		putToScreen();
    bool		hasFixedUnits() const;

    void		initFlds(CallBacker*);
    void		formChangedCB(CallBacker*);
    void		formSetCB(CallBacker*);
    void		inpSetCB(CallBacker*);
    void		subInpSetCB(CallBacker*);
    void		formTypeSetCB(CallBacker*);
    void		formMnSetCB(CallBacker*);
    void		formUnitSetCB(CallBacker*);
    void		chooseUnitsCB(CallBacker*);
    void		recButPush(CallBacker*);
    void		readReq(CallBacker*);
    void		writeReq(CallBacker*);

public:

    mDeprecated("Use MnemonicSelection")
    bool		useForm(const TypeSet<Mnemonic::StdType>*);
};
