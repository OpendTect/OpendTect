#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.5 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IODirEntryList;
class IOObj;
class uiButtonGroup;
class uiIOObjManipGroup;
class uiListBox;
class uiTextEdit;
class uiToolButton;


/*! \brief
Well manager
*/

class uiWellMan : public uiDialog
{
public:
    				uiWellMan(uiParent*);
				~uiWellMan();

    Notifier<uiWellMan>		markerschanged;

protected:

    IODirEntryList*		entrylist;
    uiListBox*			listfld;
    uiTextEdit*			infofld;
    uiIOObjManipGroup*		manipgrp;
    uiListBox*			logsfld;
    uiToolButton*		rembut;
    uiButtonGroup*		butgrp;

    CtxtIOObj&			ctio;

    void			selChg(CallBacker*);
    void			mkFileInfo();
    BufferString		getFileSize(const char*);
    void			fillLogsFld();
    void			removeLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);

    void			addMarkers(CallBacker*);
    void			addLogs(CallBacker*);
};


#endif
