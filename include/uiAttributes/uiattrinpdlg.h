#ifndef uiattrinpdlg_h
#define uiattrinpdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiattrinpdlg.h,v 1.1 2005-06-09 13:12:35 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiTextEdit;
class uiSeisSel;
class CtxtIOObj;
class BufferStringSet;


class uiAttrInpDlg : public uiDialog
{
public:
			uiAttrInpDlg(uiParent*,const BufferStringSet&,bool);
			~uiAttrInpDlg();

    void		set2DPol(Pol2D);
    bool		is2D() const;

    const char*		getUserRef();
    const char*		getDefStr();

protected:

    uiTextEdit*		txtfld;
    uiSeisSel*		inpfld;

    CtxtIOObj&		ctio;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
