#ifndef uifileman_h
#define uifileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.5 2003-05-20 12:42:12 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class CtxtIOObj;
class uiListBox;
class uiTextEdit;
class uiToolButton;
class IODirEntryList;
class uiIOObjManipGroup;


class uiSeisFileMan : public uiDialog
{
public:
			uiSeisFileMan(uiParent*);

protected:

    IODirEntryList*	entrylist;
    uiListBox*		listfld;
    uiTextEdit*		infofld;
    uiToolButton*	mergebut;
    uiToolButton*	copybut;
    uiIOObjManipGroup*	manipgrp;

    IOObj*		ioobj;
    CtxtIOObj&		ctio;

    void		selChg(CallBacker*);
    void		mergePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		relocMsg(CallBacker*);
    void		mkFileInfo();
    BufferString	getFileSize(const char*);


};


#endif
