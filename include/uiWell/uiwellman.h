#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.3 2003-10-16 15:01:37 nanne Exp $
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
