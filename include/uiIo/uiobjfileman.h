#ifndef uiobjfileman_h
#define uiobjfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiobjfileman.h,v 1.1 2004-10-28 15:01:14 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IODirEntryList;
class uiIOObjManipGroup;
class uiGroup;
class uiListBox;
class uiTextEdit;


class uiObjFileMan : public uiDialog
{
public:
				uiObjFileMan(uiParent*,const uiDialog::Setup&,
					     CtxtIOObj&);
				~uiObjFileMan();

    static BufferString		getFileSizeString(double);

protected:

    uiGroup*			topgrp;
    IODirEntryList*		entrylist;
    uiListBox*			listfld;
    uiTextEdit*			infofld;
    uiIOObjManipGroup*		manipgrp;

    CtxtIOObj&			ctio;

    void			createDefaultUI(const char* extension);
    BufferString		getFileInfo();
    virtual void		mkFileInfo()			=0;
    virtual double		getFileSize(const char*);

    void			selChg(CallBacker*);
    virtual void		ownSelChg()			{}
    virtual void		rightClicked(CallBacker*)	{}

    virtual void		relocMsg(CallBacker*);
    virtual void		postReloc(CallBacker*);
};


#endif
