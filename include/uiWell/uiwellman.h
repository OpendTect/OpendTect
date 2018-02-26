#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiobjfileman.h"
#include "wellcommon.h"
#include "bufstringset.h"
#include "dbkey.h"

namespace Well { class Data; }
class uiListBox;
class uiButton;
class uiGroup;
class uiToolButton;


mExpClass(uiWell) uiWellMan : public uiObjFileMan
{ mODTextTranslationClass(uiWellMan)
public:
				uiWellMan(uiParent*);
				~uiWellMan();

    mDeclInstanceCreatedNotifierAccess(uiWellMan);

    const DBKeySet&		getSelWells() const	{ return selwellids_; }
    const BufferStringSet&	getAvailableLogs() const
							{ return avlognms_; }
    void			getSelLogs(BufferStringSet&) const;

    static void			setButToolTip(uiButton* but,
				const uiString& oper,const uiString& objtyp,
				const uiString& obj,
				const uiString& end=uiString::empty());

protected:

    uiListBox*			logsfld_;
    uiGroup*			logsgrp_;

    bool			curiswritable_;
    DBKeySet			selwellids_;
    BufferStringSet		avlognms_;

    uiToolButton*		logvwbut_;
    uiToolButton*		logrenamebut_;
    uiToolButton*		logrmbut_;
    uiToolButton*		logexpbut_;
    uiToolButton*		loguombut_;
    uiToolButton*		logedbut_;
    uiToolButton*		logupbut_;
    uiToolButton*		logdownbut_;
    uiButton*			addlogsbut_;
    uiButton*			calclogsbut_;
    uiToolButton*		welltrackbut_;
    uiToolButton*		d2tbut_;
    uiToolButton*		csbut_;
    uiToolButton*		markerbut_;

    void			setWellToolButtonProperties();
    void			setLogToolButtonProperties();
    void			ownSelChg();
    virtual bool		gtItemInfo(const IOObj&,uiPhraseSet&) const;
    void			fillLogsFld();
    RefMan<Well::Data>		getWellData(DBKey,bool emiterr,
					    Well::SubObjType t1=Well::Inf,
					    Well::SubObjType t2=Well::Inf);
    void			saveWell(const Well::Data&,bool showwait=true);
    void			defD2T(bool);

    void			bulkD2TCB(CallBacker*);
    void			viewLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);
    void			removeLogPush(CallBacker*);
    void			editLogPush(CallBacker*);
    void			moveLogPush(CallBacker*);
    void			logSel(CallBacker*);
    void			logUOMPush(CallBacker*);
    void			itmChosenCB(CallBacker*)    { ownSelChg(); }

    void			edMarkers(CallBacker*);
    void			edWellTrack(CallBacker*);
    void			edD2T(CallBacker*);
    void			edChckSh(CallBacker*);
    void			importLogs(CallBacker*);
    void			calcLogs(CallBacker*);
    void			exportLogs(CallBacker*);
    void			logTools(CallBacker*);

};
