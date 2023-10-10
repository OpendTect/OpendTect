#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

#include "mnemonics.h"

class uiComboBox;
class uiLabel;
class UnitOfMeasure;


/*!\brief Selector for UnitOfMeasure's

  Uses the UnitOfMeasure::currentDefaults() to get/set a table of 'last used'
  unit of measure values. This list provides the start-up seletion. Therefore,
  make sure the values you display are converted to user values after the
  uiUnitSel is constructed.

  The currentDefaults() key is taken from the label text, if you want no label
  then you probably want to set a key. If none is known then the name of the
  Property Type will be used.

  */

mExpClass(uiTools) uiUnitSel : public uiGroup
{
public:

    mExpClass(uiTools) Setup
    {
    public:

	enum Mode		{ SymbolsOnly, NamesOnly, Full };

				Setup(Mnemonic::StdType,
				     const uiString& labeltxt=uiString::empty(),
				     const Mnemonic* =nullptr);
				Setup(const uiString&,
				      const SurveyInfo* =nullptr);
				//<! For Z unit only
				~Setup();

	mDefSetupMemb(Mnemonic::StdType,ptype)
	mDefSetupMemb(const Mnemonic*,mn)
	mDefSetupMemb(uiString,lbltxt)
	mDefSetupMemb(Mode,mode)		// Full
	mDefSetupMemb(bool,selproptype)		// False
	mDefSetupMemb(bool,selmnemtype)		// false
	mDefSetupMemb(bool,variableszpol)	// false
	mDefSetupMemb(bool,allowneg)		// false
	mDefSetupMemb(bool,withnone)		// false
    };

				uiUnitSel(uiParent*,const Setup&);
				uiUnitSel(uiParent*,Mnemonic::StdType);
				uiUnitSel(uiParent*,const Mnemonic* mn=nullptr);
				~uiUnitSel();

    void			setUnit(const UnitOfMeasure* uom=nullptr);
    const UnitOfMeasure*	getUnit() const;
    const char*			getUnitName() const;

    Mnemonic::StdType		propType() const	{ return setup_.ptype_;}
    void			setPropType(Mnemonic::StdType);

    const Mnemonic*		mnemonic() const;
    bool			hasMnemonicSelection() const { return mnfld_; }
    void			setMnemonic(const Mnemonic&);

    void			fillPar(IOPar&,
					const char* altkey=nullptr) const;
    bool			usePar(const IOPar&,const char* altkey=nullptr);

    const char*			tblKey() const;
				    //!< For UnitOfMeasure::currentDefaults()
				    //!< default is prop std nm, else lbltxt
    void			setFallbackKey(const char*);
    CNotifier<uiUnitSel,const UnitOfMeasure*> selChange;
				//!< Returns previous unit
    Notifier<uiUnitSel>		propSelChange;

    static IOPar&		lastUsed();
				    //!< == UnitOfMeasure::currentDefaults()

private:

    Setup			setup_;
    ObjectSet<const UnitOfMeasure> units_;
    mutable const UnitOfMeasure* prevuom_ = nullptr;
    BufferString		tblkey_;

    uiLabel*			inplbl_ = nullptr;
    uiComboBox*			inpfld_;
    uiComboBox*			propfld_ = nullptr;
    uiComboBox*			mnfld_ = nullptr;

    void			initGrp(CallBacker*);
    void			selChg(CallBacker*);
    void			propSelChg(CallBacker*);
    void			mnSelChg(CallBacker*);

    void			setPropFld(Mnemonic::StdType);
    void			setMnemFld(const Mnemonic*);
    void			setUnFld(const UnitOfMeasure*);
    void			update();
    void			displayGroup(bool yn);

    uiString			getSelTxt(const UnitOfMeasure*) const;
    const UnitOfMeasure*	gtUnit() const;

private:

    void			init();

public:

    mDeprecatedDef		uiUnitSel(uiParent* p,
					  const char* lbltxt=nullptr);

    mDeprecatedDef
    void			setUnit(const char*);
    mDeprecatedDef
    float			getUserValue(float internalval) const;
    mDeprecatedDef
    double			getUserValue(double internalval) const;
    mDeprecatedDef
    float			getInternalValue(float uservalue) const;
    mDeprecatedDef
    double			getInternalValue(double uservalue) const;
};
