#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.18 2009-10-29 09:31:04 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
class uiListBox;
class uiButton;
class uiGroup;
namespace Well { class Data; class Reader; };


mClass uiWellMan : public uiObjFileMan
{
public:
    				uiWellMan(uiParent*);
				~uiWellMan();

    static NotifierAccess*	fieldsCreated();
    void			addTool(uiButton*);

protected:

    uiListBox*			logsfld_;
    uiGroup*			logsgrp_;
    uiButton*			lastexternal_;

    Well::Data*			curwd_;
    Well::Reader*		currdr_;
    BufferString		curfnm_;

    void			ownSelChg();
    void			getCurrentWell();
    void			mkFileInfo();
    void			writeLogs();
    void			fillLogsFld();
    void			removeLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);

    void			edMarkers(CallBacker*);
    void			edWellTrack(CallBacker*);
    void			edD2T(CallBacker*);
    void			edChckSh(CallBacker*);
    void			importLogs(CallBacker*);
    void			calcLogs(CallBacker*);
    void			exportLogs(CallBacker*);

    double			getFileSize(const char*,int&) const;
    void			defD2T(bool);

};

#endif
