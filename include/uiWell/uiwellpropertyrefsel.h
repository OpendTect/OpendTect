#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          April 2011 / May 2014
________________________________________________________________________

-*/

#include "multiid.h"
#include "mnemonics.h"
#include "welllogset.h"
#include "uigroup.h"
#include "uiwellmod.h"
#include "uistring.h"

class UnitOfMeasure;

class uiLabel;
class uiComboBox;
class uiCheckBox;
class uiButton;
class uiUnitSel;

namespace Well { class LogSet; }


mExpClass(uiWell) uiWellSingleMnemSel : public uiGroup
{ mODTextTranslationClass(uiWellSingleMnemSel);
public:
			uiWellSingleMnemSel(uiParent*,const Mnemonic&,
					const Mnemonic* alternatemn=0);

    bool		setAvailableLogs(const Well::LogSet&);

    void                set(const char* txt,bool alt,const UnitOfMeasure* u=0);
    void                setCurrent(const char*);
    void                setUOM(const UnitOfMeasure&);

    const char*         logName() const;
    const UnitOfMeasure* getUnit() const;

    void		selectAltMnem(bool yn);
    bool		altMnemSelected() const;

    const Mnemonic&	normMnem() const	{ return mnem_; }
    const Mnemonic*	altMnem() const		{ return altmnem_; }
    const Mnemonic&	selMnem() const;

    Notifier<uiWellSingleMnemSel> altMnemChosen;

protected:

    const Mnemonic&	mnem_;
    const Mnemonic*	altmnem_;
    BufferStringSet	normnms_, normunmeaslbls_;
    BufferStringSet	altnms_, altunmeaslbls_;
    int			altmn_;

    uiComboBox*         lognmfld_;
    uiUnitSel*          unfld_;
    uiCheckBox*         altbox_;

    void		updateSelCB(CallBacker*);
    void		switchMnemCB(CallBacker*);

    void                updateLogInfo();

};


mExpClass(uiWell) uiWellMnemSel : public uiGroup
{ mODTextTranslationClass(uiWellMnemSel);
public:

			uiWellMnemSel(uiParent*,const MnemonicSelection&);
    int			size() const	{ return mnemflds_.size(); }

    bool		setAvailableLogs(const Well::LogSet&,
	    				 BufferStringSet& notokpropnms);
    void		setLog(const PropertyRef::StdType,const char*,
				bool check,const UnitOfMeasure*, int idx);
    bool		getLog(const PropertyRef::StdType,BufferString&,
				bool&, BufferString& uom, int idx) const;

    uiWellSingleMnemSel* getMnemSelFromListByName(const BufferString&);
    uiWellSingleMnemSel* getMnemSelFromListByIndex(int);
    virtual bool	isOK() const;
    void		setWellID( const MultiID& wid ) { wellid_ = wid; }

    uiButton*		getRightmostButton( int idx ) { return viewbuts_[idx]; }

    MultiID		wellid_;
    Notifier<uiWellMnemSel> logCreated;

protected:

    ObjectSet<uiWellSingleMnemSel> mnemflds_;
    ObjectSet<uiButton> createbuts_;
    ObjectSet<uiButton> viewbuts_;

    void		updateSelCB(CallBacker*);
    void		createLogPushed(CallBacker*);
    void		viewLogPushed(CallBacker*);

};


