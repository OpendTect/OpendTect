#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.7 2004-04-29 16:41:46 nanne Exp $
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

namespace Well { class Data; class Reader; };


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
    Well::Data*			welldata;
    Well::Reader*		wellrdr;
    BufferString		fname;

    void			selChg(CallBacker*);
    void			getCurrentWell();
    void			mkFileInfo();
    BufferString		getFileSize(const char*);
    void			fillLogsFld();
    void			removeLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);

    void			addMarkers(CallBacker*);
    void			addLogs(CallBacker*);
    void			exportLogs(CallBacker*);
};


#endif
