#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.2 2003-10-16 09:41:18 bert Exp $
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
    void		fillAttribList(const ObjectSet<BufferString>&);

};


#endif
