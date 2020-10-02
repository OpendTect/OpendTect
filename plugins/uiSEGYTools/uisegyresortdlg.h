#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2011
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
class uiIOObjSel;
class uiGenInput;
class uiPosSubSel;
class uiFileSel;
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
    uiFileSel*		outfld_;

    uiIOObjSel*		objSel();

    void		inpSel(CallBacker*);
    void		geomSel(CallBacker*);
    void		nrinlSel(CallBacker*);
    bool		acceptOK();

};
