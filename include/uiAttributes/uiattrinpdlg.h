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
			//!<Use for single input case
			~uiAttrInpDlg();

    bool		is2D() const 		{ return is2d_ ; }

    const char*		getSeisRefFromIndex(int) const;
    const char*		getSteerRefFromIndex(int) const;
    const char*		getSeisKeyFromIndex(int) const;
    const char*		getSteerKeyFromIndex(int) const;

    //old functions, will be removed after version 6.0
    const char*		getSeisRef() const;
    const char*		getSteerRef() const;
    const char*		getSeisKey() const;
    const char*		getSteerKey() const;

protected:

    ObjectSet<uiSeisSel>	seisinpflds_;
    ObjectSet<uiSeisSel>	steerinpflds_;

    bool		multiinpcube_;
    bool		is2d_;

    bool		acceptOK(CallBacker*) override;

};

