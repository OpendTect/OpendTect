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
class uiCheckBox;
class uiComboBox;
class uiLabel;
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
    void		setNonSpecInputs(const BufferStringSet&,int iinp=-1,
					 const MnemonicSelection* =nullptr);
    void		setNonSpecSubInputs(const BufferStringSet&,int iinp=-1);

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
    CNotifier<uiMathFormula,const Mnemonic*> formMnSet;

    uiMathExpression*	exprFld()		{ return exprfld_; }
    int			nrInpFlds() const	{ return inpflds_.size(); }
    uiMathExpressionVariable* inpFld( int idx )	{ return inpflds_[idx]; }
    int			vwLogInpNr(CallBacker*) const;
    bool		checkValidNrInputs() const;

private:
			mOD_DisableCopy(uiMathFormula);

    Math::Formula&	form_;
    Setup		setup_;
    TypeSet<double>	recvals_;

    uiMathExpression*	exprfld_;
    MnemonicSelection*	mnsel_ = nullptr;
    uiToolButton*	recbut_ = nullptr;
    bool		fullformupdate_ = false;
    uiLabel*		formlbl_ = nullptr;
    ObjectSet<uiMathExpressionVariable> inpflds_;

    uiLabel*		formreslbl_ = nullptr;
    uiComboBox*		typfld_ = nullptr;
    ObjectSet<uiMnemonicsSel> mnselflds_;
    ObjectSet<uiUnitSel> unitflds_;
    uiCheckBox*		selectunitsfld_ = nullptr;

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
    void		guessOutputFormDefs();
    bool		setOutputDefsFromForm(bool hasfixedunits);
    bool		putToScreen();
    bool		hasFixedUnits() const;

    void		initGrp(CallBacker*);
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

    mDeprecated("Use Math::Formula::allChanged")
    bool		useForm();

    mDeprecatedObs
    void		setFixedFormUnits(bool)			{}
};
