#ifndef uiattrinpdlg_h
#define uiattrinpdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiattrinpdlg.h,v 1.5 2007-05-22 07:36:43 cvsnanne Exp $
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
			uiAttrInpDlg(uiParent*,const BufferStringSet& attrnms,
				     bool issteer,bool is2d);
			~uiAttrInpDlg();

    bool		is2D() const;

    const char*		getUserRef() const;
    const char*		getKey() const;

protected:

    uiTextEdit*		txtfld;
    uiSeisSel*		inpfld;

    CtxtIOObj&		ctio;
    bool		issteer_;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
