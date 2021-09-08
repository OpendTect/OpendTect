#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiobjfileman.h"
#include "bufstringset.h"

class uiListBox;
class uiButton;
class uiGroup;
class uiToolButton;
class uiPushButton;
namespace Well { class Data; class Reader; }
class uiWellLogCalc;

mExpClass(uiWell) uiWellMan : public uiObjFileMan
{ mODTextTranslationClass(uiWellMan)
public:
				uiWellMan(uiParent*);
				~uiWellMan();

    mDeclInstanceCreatedNotifierAccess(uiWellMan);

    const TypeSet<MultiID>&	getSelWells() const	{ return curmultiids_; }
    void			getSelLogs(BufferStringSet&) const;
    const BufferStringSet&	getAvailableLogs() const;
    static void			setButToolTip(uiButton* but,
				const uiString& oper,const uiString& objtyp,
				const uiString& obj,
				const uiString& end=uiStrings::sEmptyString());

protected:

    uiListBox*			logsfld_;
    uiGroup*			logsgrp_;

    bool			iswritable_;
    ObjectSet<Well::Data>	curwds_;
    ObjectSet<Well::Reader>	currdrs_;
    TypeSet<MultiID>		curmultiids_;
    BufferStringSet		curfnms_;
    BufferStringSet		availablelognms_;

    uiToolButton*		logvwbut_;
    uiToolButton*		logrenamebut_;
    uiToolButton*		logrmbut_;
    uiToolButton*		logexpbut_;
    uiToolButton*		loguombut_;
    uiToolButton*		logedbut_;
    uiToolButton*		logupbut_;
    uiToolButton*		logdownbut_;
    uiPushButton*		addlogsbut_;
    uiPushButton*		calclogsbut_;
    uiToolButton*		welltrackbut_;
    uiToolButton*		d2tbut_;
    uiToolButton*		csbut_;
    uiToolButton*		markerbut_;

    uiWellLogCalc*		welllogcalcdlg();
    void			updateLogsFld(CallBacker*);
    void			calcClosedCB(CallBacker*);

    void			setWellToolButtonProperties();
    void			setLogToolButtonProperties();
    void			ownSelChg();
    void			getCurrentWells();
    void			mkFileInfo();
    void			writeLogs();
    void			fillLogsFld();
    void			wellsChgd();
    void			copyPush(CallBacker*);
    void			bulkD2TCB(CallBacker*);
    void			viewLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);
    void			removeLogPush(CallBacker*);
    void			editLogPush(CallBacker*);
    void			moveLogsPush(CallBacker*);
    void			logSel(CallBacker*);
    void			logUOMPush(CallBacker*);

    void			edMarkers(CallBacker*);
    void			edWellTrack(CallBacker*);
    void			edD2T(CallBacker*);
    void			edChckSh(CallBacker*);
    void			importLogs(CallBacker*);
    void			calcLogs(CallBacker*);
    void			exportLogs(CallBacker*);
    void			logTools(CallBacker*);

    void			defD2T(bool);

};

