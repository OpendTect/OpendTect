#ifndef uiattrinpdlg_h
#define uiattrinpdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiattrinpdlg.h,v 1.6 2008-06-18 08:20:40 cvssatyaki Exp $
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
				     bool issteer,bool is2d,bool multiinpcube);
			~uiAttrInpDlg();

    bool		is2D() const;

    const char*		getUserRef() const;
    const char*		getSeisRef() const;
    const char*		getSteerRef() const;
    const char*		getKey() const;
    const char*		getSeisKey() const;
    const char*		getSteerKey() const;

protected:

    uiTextEdit*		txtfld;
    uiSeisSel*		inpfld;
    uiSeisSel*		seisinpfld;
    uiSeisSel*		steerinpfld;

    CtxtIOObj&		ctio;
    CtxtIOObj&		ctiosteer;
    bool		issteer_;
    bool		multiinpcube_;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
