#ifndef uifileman_h
#define uifileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.3 2002-06-26 16:34:41 bert Exp $
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
    uiPushButton*	copybut;

    IOObj*		ioobj;
    CtxtIOObj&		ctio;

    void		selChg(CallBacker*);
    void		removePush(CallBacker*);
    void		renamePush(CallBacker*);
    void		mergePush(CallBacker*);
    void		relocatePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		refreshList(int);
    void		mkFileInfo();
    void		handleMultiFiles(const char*,const char*);


};


#endif
