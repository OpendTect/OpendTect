#ifndef uiempreloaddlg_h
#define uiempreloaddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id: uiempreloaddlg.h,v 1.2 2010-08-25 08:33:39 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiListBox;
class uiTextEdit;
class BufferString;

mClass uiEMPreLoadDlg : public uiDialog
{
public:
			uiEMPreLoadDlg( uiParent* p, const char* title )
			    : uiDialog(p,Setup("Pre-load manager"
				       ,title, "103.0.13") )	{}
protected:
    virtual void	unloadPushCB(CallBacker*)		=0;
    virtual void	selCB(CallBacker*)			=0;

    uiListBox*		listfld_;
    uiTextEdit*		infofld_;
};

mClass uiHorizonPreLoadDlg : public uiEMPreLoadDlg
{
public:
			uiHorizonPreLoadDlg(uiParent*,const char*);
protected:
    bool		add3DPushCB(CallBacker*);
    void		unloadPushCB(CallBacker*);
    void		selCB(CallBacker*);
    void		openPushCB(CallBacker*);
    void		savePushCB(CallBacker*);
};


#endif
