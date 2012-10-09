#ifndef uiattrinpdlg_h
#define uiattrinpdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id$
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
				     bool issteer,bool is2d,
				     const char* prevrefnm =0);
			//!<Use for multi input case
			uiAttrInpDlg(uiParent*,bool hasseis,
				     bool hassteer,bool is2d);
			//!<Use for single input case
			~uiAttrInpDlg();

    bool		is2D() const 		{ return is2d_ ; }

    const char*		getSeisRef() const;
    const char*		getSteerRef() const;
    const char*		getSeisKey() const;
    const char*		getSteerKey() const;

protected:

    uiSeisSel*		seisinpfld_;
    uiSeisSel*		steerinpfld_;

    CtxtIOObj&		ctio_;
    CtxtIOObj&		ctiosteer_;
    bool		multiinpcube_;
    bool		is2d_;

    bool		acceptOK(CallBacker*);
    CtxtIOObj& 		getCtxtIO(bool);

};

#endif
