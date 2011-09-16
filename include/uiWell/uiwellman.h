#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.25 2011-09-16 10:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
#include "bufstringset.h"

class uiListBox;
class uiButton;
class uiGroup;
class uiPushButton;
namespace Well { class Data; class Reader; };


mClass uiWellMan : public uiObjFileMan
{
public:
    				uiWellMan(uiParent*);
				~uiWellMan();

    mDeclInstanceCreatedNotifierAccess(uiWellMan);
    void			addTool(uiButton*);

protected:

    uiListBox*			logsfld_;
    uiGroup*			logsgrp_;

    ObjectSet<Well::Data>	curwds_;
    ObjectSet<Well::Reader>	currdrs_;
    TypeSet<MultiID>		multiids_;
    BufferStringSet		curfnms_;

    uiToolButton*		logupbut_;
    uiToolButton*		logdownbut_;
    uiPushButton*		addlogsbut_;
    uiPushButton*		calclogsbut_;

    void			checkButtons();
    void			ownSelChg();
    void			getCurrentWells();
    void			mkFileInfo();
    void			writeLogs();
    void			fillLogsFld();
    void			removeLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);
    void			moveLogsPush(CallBacker*);
    void			checkMoveLogs(CallBacker*);
    void			logUOMPush(CallBacker*);

    void			edMarkers(CallBacker*);
    void			edWellTrack(CallBacker*);
    void			edD2T(CallBacker*);
    void			edChckSh(CallBacker*);
    void			importLogs(CallBacker*);
    void			calcLogs(CallBacker*);
    void			exportLogs(CallBacker*);
    void			logTools(CallBacker*);

    double			getFileSize(const char*,int&) const;
    void			defD2T(bool);

};

#endif
