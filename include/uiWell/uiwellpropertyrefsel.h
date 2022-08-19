#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uigroup.h"
#include "uistring.h"

#include "mnemonics.h"
#include "multiid.h"

class PropertyRef;
class PropertyRefSelection;
class UnitOfMeasure;

class uiButton;
class uiCheckBox;
class uiComboBox;
class uiLabel;
class uiLabeledComboBox;
class uiUnitSel;

namespace Well { class LogSet; }


mClass(uiWell) uiWellSinglePropSel : public uiGroup
{ mODTextTranslationClass(uiWellSinglePropSel);
public:
			uiWellSinglePropSel(uiParent*,const Mnemonic&);
			uiWellSinglePropSel(uiParent*,const PropertyRef&);
			~uiWellSinglePropSel();

    bool		isOK() const;

    bool		setAvailableLogs(const Well::LogSet&);
    bool		setDefaultLog(const Well::LogSet&,const Mnemonic&);

    void                set(const char* txt,bool alt,
			    const UnitOfMeasure* =nullptr);
    void                setCurrent(const char*);
    void                setUOM(const UnitOfMeasure*);

    const char*         logName() const;
    const UnitOfMeasure* getUnit() const;

    void		selectAltProp(bool yn);
    bool		altPropSelected() const;

    const char*		logtypeName() const	{ return logtypename_.buf(); }
    const char*		altLogtypeName() const	{ return altlogtypename_.buf();}
    const char*		selLogtypeName() const;

    const Mnemonic&	normMn() const;
    const Mnemonic*	altMn() const;
    const Mnemonic&	selMn() const;

private:

    const BufferString	logtypename_;
    BufferString	altlogtypename_;
    const PropertyRef*	propref_ = nullptr;
    const PropertyRef*	altpropref_ = nullptr;

    uiLabeledComboBox*	lognmfld_;
    uiUnitSel*          unfld_;
    ObjectSet<const UnitOfMeasure> logunits_;
    uiCheckBox*         altbox_ = nullptr;
    uiLabeledComboBox*	altlognmfld_ = nullptr;
    uiUnitSel*          altunfld_ = nullptr;
    ObjectSet<const UnitOfMeasure> altlogunits_;

    static const Mnemonic*	guessAltMnemonic(const Mnemonic&);
    static const PropertyRef*	guessAltProp(const PropertyRef&);

    void		makeLogNameFld(const UnitOfMeasure*);
    void		makeAltLogNameFld(const UnitOfMeasure*);

    void		switchPropCB(CallBacker*);
    void		updateSelCB(CallBacker*);

    uiComboBox*		curLogNmFld() const;
    uiUnitSel*		curUnitFld() const;

public:
			mDeprecatedDef
			uiWellSinglePropSel(uiParent*,const PropertyRef&,
					    const PropertyRef* alternatepr);

    mDeprecatedDef
    const PropertyRef&	normPropRef() const;
    mDeprecatedDef
    const PropertyRef*	altPropRef() const;
    mDeprecatedDef
    const PropertyRef&	selPropRef() const;

};


mExpClass(uiWell) uiWellPropSel : public uiGroup
{ mODTextTranslationClass(uiWellPropSel);
public:

			uiWellPropSel(uiParent*,const MnemonicSelection&);
			uiWellPropSel(uiParent*,const PropertyRefSelection&);
			~uiWellPropSel();

    bool		isOK() const;
    int			size() const	{ return propflds_.size(); }

    void		setWellID( const MultiID& wid ) { wellid_ = wid; }
    bool		setAvailableLogs(const Well::LogSet&,
					 BufferStringSet& notokpropnms);
    void		setLog(const Mnemonic*,const char* lognm,
			       bool check,const UnitOfMeasure*,int idx);

    const Mnemonic*	getMnRef(int idx) const;
    const char*		getLogTypename(int idx,bool selected=false) const;
    bool		getLog(const Mnemonic&,BufferString&,
			       bool&,const UnitOfMeasure*& uom,int idx) const;

    uiButton*		getRightmostButton( int idx ) { return viewbuts_[idx]; }

    Notifier<uiWellPropSel> logCreated;

private:

    MultiID		wellid_;

    ObjectSet<uiWellSinglePropSel> propflds_;
    ObjectSet<uiButton> createbuts_;
    ObjectSet<uiButton> viewbuts_;

    void		addButtons();
    void		createLogPushed(CallBacker*);
    void		viewLogPushed(CallBacker*);

public:

    mDeprecated("Use Mnemonic")
    void		setLog(const Mnemonic::StdType,const char* lognm,
			       bool check,const UnitOfMeasure*,int idx);
    mDeprecated("Use Mnemonic")
    bool		getLog(const Mnemonic::StdType,BufferString&,
			       bool&,BufferString& uom,int idx) const;

    mDeprecatedDef
    uiWellSinglePropSel* getPropSelFromListByName(const BufferString&);

    mDeprecatedDef
    uiWellSinglePropSel* getPropSelFromListByIndex(int);

};
