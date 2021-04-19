#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistrings.h"
#include "propertyref.h"
#include "mnemonics.h"

class uiComboBox;
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

				Setup( PropertyRef::StdType st,
				       const uiString labeltxt=
				       uiStrings::sEmptyString(),
				       Mnemonic* mn = nullptr )
				    : ptype_(st)
				    , mn_(mn)
				    , lbltxt_(mToUiStringTodo(labeltxt))
				    , mode_(Full)
				    , selproptype_(false)
				    , selmnemtype_(mn ? true : false)
				    , withnone_(false)	{}

	mDefSetupMemb(PropertyRef::StdType,ptype)
	mDefSetupMemb(Mnemonic*,mn)
	mDefSetupMemb(uiString,lbltxt)
	mDefSetupMemb(Mode,mode)
	mDefSetupMemb(bool,selproptype)
	mDefSetupMemb(bool,selmnemtype)
	mDefSetupMemb(bool,withnone)
    };

				uiUnitSel(uiParent*,const Setup&);
				uiUnitSel(uiParent*,PropertyRef::StdType);
				uiUnitSel(uiParent*,Mnemonic* mn=nullptr);
				uiUnitSel(uiParent*,const char* lbltxt=0);
					//!< For survey Z unit
				~uiUnitSel();

    void			setUnit(const UnitOfMeasure* uom=0);
    void			setUnit(const char*);
    const UnitOfMeasure*	getUnit() const;
    const char*			getUnitName() const;

    float			getUserValue(float internalval) const;
    double			getUserValue(double internalval) const;
    float			getInternalValue(float uservalue) const;
    double			getInternalValue(double uservalue) const;


    PropertyRef::StdType	propType() const	{ return setup_.ptype_;}
    void			setPropType(PropertyRef::StdType);

    Mnemonic*			mnemonic() const;
    void			setMnemonic(Mnemonic&);

    uiComboBox*			inpFld() const	{ return inpfld_; }

    Notifier<uiUnitSel>		selChange;
    Notifier<uiUnitSel>		propSelChange;

    void			fillPar(IOPar&,const char* altkey=0) const;
    bool			usePar(const IOPar&,const char* altkey=0);

    const char*			tblKey() const;
				    //!< For UnitOfMeasure::currentDefaults()
				    //!< default is prop std nm, else lbltxt
    void			setFallbackKey(const char*);
    static IOPar&		lastUsed();
				    //!< == UnitOfMeasure::currentDefaults()

protected:

    Setup			setup_;
    ObjectSet<const UnitOfMeasure> units_;
    BufferString		tblkey_;

    uiComboBox*			inpfld_;
    uiComboBox*			propfld_ = nullptr;
    uiComboBox*			mnfld_ = nullptr;

    void			selChg( CallBacker* )	{ selChange.trigger(); }
    void			propSelChg(CallBacker*);
    void			mnSelChg(CallBacker*);
    void			setPropFld(PropertyRef::StdType);
    void			setMnemFld(Mnemonic*);
    void			setUnFld(const UnitOfMeasure*);
    void			update();
    uiString			getSelTxt(const UnitOfMeasure*) const;
    const UnitOfMeasure*	gtUnit() const;

private:

    void			init();

};


