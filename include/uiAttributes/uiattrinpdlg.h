#ifndef uiattrinpdlg_h
#define uiattrinpdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiattrinpdlg.h,v 1.10 2009-02-02 11:36:23 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiTextEdit;
class uiSeisSel;
class CtxtIOObj;
class BufferStringSet;


mClass uiAttrInpDlg : public uiDialog
{
public:
			uiAttrInpDlg(uiParent*,const BufferStringSet& attrnms,
				     bool issteer,bool is2d);
			//!<Use for multi input case
			uiAttrInpDlg(uiParent*,bool hasseis,
				     bool hassteer,bool is2d);
			//!<Use for single input case
			~uiAttrInpDlg();

    bool		is2D() const 		{ return is2d_ ; }

    void 		setCtxtIO();
    const char*		getUserRef() const;
    const char*		getSeisRef() const;
    const char*		getSteerRef() const;
    const char*		getKey() const;
    const char*		getSeisKey() const;
    const char*		getSteerKey() const;

protected:

    uiSeisSel*		inpfld_;
    uiSeisSel*		seisinpfld_;
    uiSeisSel*		steerinpfld_;

    CtxtIOObj&		ctio_;
    CtxtIOObj&		ctiosteer_;
    bool		multiinpcube_;
    bool		is2d_;

    bool		acceptOK(CallBacker*);
};

#endif
