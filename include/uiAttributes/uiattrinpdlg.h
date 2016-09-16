#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

class uiTextEdit;
class uiSeisSel;
class BufferStringSet;


mExpClass(uiAttributes) uiAttrInpDlg : public uiDialog
{ mODTextTranslationClass(uiAttrInpDlg);
public:
			uiAttrInpDlg(uiParent*,
				     const BufferStringSet& seisinpnms,
				     const BufferStringSet& steeringinpnms,
				     bool is2d);
			//!<Use for multi input case
			uiAttrInpDlg(uiParent*,const BufferStringSet& attrnms,
				     bool issteer,bool is2d,
				     const char* prevrefnm =0);
			//!<Use for single or multi input case - old fashioned
			~uiAttrInpDlg();

    bool		is2D() const		{ return is2d_ ; }

    const char*		getSeisRef(int) const;
    const char*		getSteerRef(int) const;
    DBKey		getSeisKey(int) const;
    DBKey		getSteerKey(int) const;

protected:

    ObjectSet<uiSeisSel>	seisinpflds_;
    ObjectSet<uiSeisSel>	steerinpflds_;

    bool		is2d_;

    bool		acceptOK();

};
