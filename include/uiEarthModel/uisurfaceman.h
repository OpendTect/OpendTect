#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.5 2004-03-31 11:13:51 nanne Exp $
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
class BufferStringSet;


class uiSurfaceMan : public uiDialog
{
public:
			uiSurfaceMan(uiParent*,bool);
			~uiSurfaceMan();

protected:

    IODirEntryList*	entrylist;
    uiListBox*		listfld;
    uiTextEdit*		infofld;
    uiIOObjManipGroup*	manipgrp;
    uiListBox*		attribfld;
    uiToolButton*	rembut;
    uiButtonGroup*	butgrp;

    CtxtIOObj&		ctio;

    void		remPush(CallBacker*);
    void		selChg(CallBacker*);
    void		relocMsg(CallBacker*);
    void		postReloc(CallBacker*);
    void		mkFileInfo();
    BufferString	getFileSize(const char*);
    void		fillAttribList(const BufferStringSet&);

};


#endif
