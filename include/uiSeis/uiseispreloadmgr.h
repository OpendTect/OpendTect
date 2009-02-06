#ifndef uiseispreloadmgr_h
#define uiseispreloadmgr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2009
 RCS:           $Id: uiseispreloadmgr.h,v 1.1 2009-02-06 14:48:27 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"
class uiListBox;
class uiTextEdit;


/*!\brief Shows loaded plugins and allows adding */

mClass uiSeisPreLoadMgr : public uiDialog
{ 	
public:
			uiSeisPreLoadMgr(uiParent*);

protected:

    uiListBox*		listfld_;
    uiTextEdit*		infofld_;
    BufferStringSet	ids_;

    void		fillList();
    void		initWin(CallBacker*);
    void		selChg(CallBacker*);
    void		cubeLoadPush(CallBacker*);
    void		linesLoadPush(CallBacker*);
    void		ps3DPush(CallBacker*);
    void		ps2DPush(CallBacker*);
    void		unloadPush(CallBacker*);

};


#endif
