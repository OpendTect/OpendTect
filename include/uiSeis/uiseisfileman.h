#ifndef uifileman_h
#define uifileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.1 2002-05-07 16:04:18 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class IODirEntryList;
class uiGenInput;
class uiListBox;
class uiPushButton;
class uiTextEdit;


class uiSeisFileMan : public uiDialog
{
public:
			uiSeisFileMan(uiParent*);

protected:

    IODirEntryList*	entrylist;
    uiListBox*		listfld;
    uiTextEdit*		infofld;
    uiPushButton*	rembut;
    uiPushButton*	renamebut;
    uiPushButton*	relocbut;
    uiPushButton*	mergebut;

    IOObj*		ioobj;
    CtxtIOObj&		ctio;

    void		selChg(CallBacker*);
    void		removePush(CallBacker*);
    void		renamePush(CallBacker*);
    void		mergePush(CallBacker*);
    void		relocatePush(CallBacker*);
    void		refreshList(int);
    void		mkFileInfo();
    bool		handleMultiFiles(const char*,const char*);
    void		createLinks(ObjectSet<BufferString>&,
				    ObjectSet<BufferString>&);
};


class FileNameDlg : public uiDialog
{
public:
			FileNameDlg(uiParent*,const char*);
    const char*		getNewName();

protected:
    uiGenInput*		namefld;
};

#endif
