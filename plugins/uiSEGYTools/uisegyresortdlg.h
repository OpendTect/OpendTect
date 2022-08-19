#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
class uiIOObjSel;
class uiGenInput;
class uiPosSubSel;
class uiFileInput;
class uiComboBox;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mExpClass(uiSEGYTools) uiResortSEGYDlg : public uiDialog
{ mODTextTranslationClass(uiResortSEGYDlg);
public :

			uiResortSEGYDlg(uiParent*);

    Seis::GeomType	geomType() const;
    uiString		sFldNm() { return tr("Scanned input"); }

protected:

    uiGenInput*		geomfld_;
    uiIOObjSel*		volfld_;
    uiIOObjSel*		ps3dfld_;
    uiIOObjSel*		ps2dfld_;
    uiComboBox*		linenmfld_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		newinleachfld_;
    uiGenInput*		inlnmsfld_;
    uiFileInput*	outfld_;

    uiIOObjSel*		objSel();

    void		inpSel(CallBacker*);
    void		geomSel(CallBacker*);
    void		nrinlSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};
