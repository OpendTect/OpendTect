#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.9 2004-10-07 18:27:48 nanne Exp $
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
			~uiSeisFileMan();

protected:

    IODirEntryList*	entrylist;
    uiListBox*		listfld;
    uiTextEdit*		infofld;
    uiToolButton*	mergebut;
    uiToolButton*	copybut;
    uiIOObjManipGroup*	manipgrp;

    CtxtIOObj&		ctio;

    void		selChg(CallBacker*);
    void		rightClicked(CallBacker*);
    void		mergePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		relocMsg(CallBacker*);
    void		postReloc(CallBacker*);
    void		mkFileInfo();
    BufferString	getFileSize(const char*);


};


#endif
