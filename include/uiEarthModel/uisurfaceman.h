#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.1 2003-08-06 15:10:23 nanne Exp $
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


class uiSurfaceMan : public uiDialog
{
public:
			uiSurfaceMan(uiParent*);

protected:

    IODirEntryList*	entrylist;
    uiListBox*		listfld;
    uiTextEdit*		infofld;
    uiIOObjManipGroup*	manipgrp;
    uiListBox*		attribfld;
    uiToolButton*	rembut;
    uiButtonGroup*	butgrp;

    IOObj*		ioobj;
    CtxtIOObj&		ctio;

    void		remPush(CallBacker*);
    void		selChg(CallBacker*);
    void		relocMsg(CallBacker*);
    void		postReloc(CallBacker*);
    void		mkFileInfo();
    BufferString	getFileSize(const char*);
    void		fillAttribList(const ObjectSet<BufferString>&);

};


#endif
