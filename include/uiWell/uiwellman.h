#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uiobjfileman.h"
#include "bufstringset.h"
#include "welldata.h"

class uiButton;
class uiGroup;
class uiListBox;
class uiPushButton;
class uiToolButton;
class uiWellLogCalc;

namespace Well { class Log; class Reader; }

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

    bool			iswritable_			= true;
    RefObjectSet<Well::Data>	curwds_;
    TypeSet<MultiID>		curmultiids_;
    BufferStringSet		curfnms_;
    BufferStringSet		availablelognms_;
    BufferStringSet		defaultlognms_;

    uiToolButton*		logvwbut_;
    uiToolButton*		logrenamebut_;
    uiToolButton*		logrmbut_;
    uiToolButton*		logcopybut_;
    uiToolButton*		logexpbut_;
    uiToolButton*		loguombut_;
    uiToolButton*		logmnembut_;
    uiToolButton*		defmnemlogbut_;
    uiToolButton*		logedbut_;
    uiToolButton*		custommnsbut_;
    uiPushButton*		addlogsbut_;
    uiPushButton*		calclogsbut_;
    uiToolButton*		welltrackbut_;
    uiToolButton*		d2tbut_				= nullptr;
    uiToolButton*		csbut_				= nullptr;
    uiToolButton*		markerbut_;

    uiWellLogCalc*		welllogcalcdlg_			= nullptr;
    void			updateLogsFld(CallBacker*);
    void			calcClosedCB(CallBacker*);

    void			setWellToolButtonProperties();
    void			setLogToolButtonProperties();
    void			ownSelChg() override;
    void			getCurrentWells();
    void			mkFileInfo() override;
    void			writeLogs();
    void			writeLog(const MultiID&,
					 Well::Data&,const Well::Log&);
    void			fillLogsFld();
    void			wellsChgd();
    void			wellLogsChgd(const BufferStringSet& lognms);
    void			copyPush(CallBacker*);
    void			bulkD2TCB(CallBacker*);
    void			viewLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);
    void			removeLogPush(CallBacker*);
    void			copyLogPush(CallBacker*);
    void			editLogPush(CallBacker*);
    void			logSel(CallBacker*);
    void			logUOMPush(CallBacker*);
    void			logMnemPush(CallBacker*);
    void			defMnemLogPush(CallBacker*);
    void			customMnsPush(CallBacker*);

    void			edMarkers(CallBacker*);
    void			edWellTrack(CallBacker*);
    void			edD2T(CallBacker*);
    void			edChckSh(CallBacker*);
    void			importLogs(CallBacker*);
    void			calcLogs(CallBacker*);
    void			exportLogs(CallBacker*);
    void			logTools(CallBacker*);

    void			defD2T(bool);
    void			getDefaultLogsList(const IOPar&,
						   BufferStringSet&);
    void			setDefaultPixmaps();

};
