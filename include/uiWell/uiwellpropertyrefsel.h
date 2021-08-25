#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          April 2011 / May 2014
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uigroup.h"
#include "uistring.h"

#include "mnemonics.h"
#include "multiid.h"
#include "welllogset.h"

class PropertyRef;
class PropertyRefSelection;
class UnitOfMeasure;

class uiLabel;
class uiComboBox;
class uiCheckBox;
class uiButton;
class uiUnitSel;

namespace Well { class LogSet; }


mExpClass(uiWell) uiWellSinglePropSel : public uiGroup
{ mODTextTranslationClass(uiWellSinglePropSel);
public:
			uiWellSinglePropSel(uiParent*,const PropertyRef&,
				const PropertyRef* alternatepr=nullptr);
			~uiWellSinglePropSel();

    bool		setAvailableLogs(const Well::LogSet&);

    void                set(const char* txt,bool alt,const UnitOfMeasure* u=0);
    void                setCurrent(const char*);
    void                setUOM(const UnitOfMeasure&);

    const char*         logName() const;
    const UnitOfMeasure* getUnit() const;

    void		selectAltProp(bool yn);
    bool		altPropSelected() const;

    const PropertyRef&	normPropRef() const	{ return propref_; }
    const PropertyRef*	altPropRef() const	{ return altpropref_; }
    const PropertyRef&	selPropRef() const;

    Notifier<uiWellSinglePropSel> altPropChosen;

protected:

    const PropertyRef&	propref_;
    const PropertyRef*	altpropref_ = nullptr;
    BufferStringSet	normnms_, normunmeaslbls_;
    BufferStringSet	altnms_, altunmeaslbls_;
    int			altpref_ = -1;

    uiComboBox*         lognmfld_;
    uiUnitSel*          unfld_;
    uiCheckBox*         altbox_ = nullptr;

    void		updateSelCB(CallBacker*);
    void		switchPropCB(CallBacker*);

    void                updateLogInfo();

};


mExpClass(uiWell) uiWellPropSel : public uiGroup
{ mODTextTranslationClass(uiWellPropSel);
public:

			uiWellPropSel(uiParent*,const PropertyRefSelection&);
			~uiWellPropSel();

    int			size() const	{ return propflds_.size(); }

    bool		setAvailableLogs(const Well::LogSet&,
					 BufferStringSet& notokpropnms);
    void		setLog(const Mnemonic::StdType,const char*,
			       bool check,const UnitOfMeasure*,int idx);
    bool		getLog(const Mnemonic::StdType,BufferString&,
			       bool&,BufferString& uom,int idx) const;

    uiWellSinglePropSel* getPropSelFromListByName(const BufferString&);
    uiWellSinglePropSel* getPropSelFromListByIndex(int);
    virtual bool	isOK() const;
    void		setWellID( const MultiID& wid ) { wellid_ = wid; }

    uiButton*		getRightmostButton( int idx ) { return viewbuts_[idx]; }

    MultiID		wellid_;
    Notifier<uiWellPropSel> logCreated;

protected:

    ObjectSet<uiWellSinglePropSel> propflds_;
    ObjectSet<uiButton> createbuts_;
    ObjectSet<uiButton> viewbuts_;

    void		updateSelCB(CallBacker*);
    void		createLogPushed(CallBacker*);
    void		viewLogPushed(CallBacker*);

};


