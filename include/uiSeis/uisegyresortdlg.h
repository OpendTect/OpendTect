#ifndef uisegyresortdlg_h
#define uisegyresortdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2011
 RCS:           $Id: uisegyresortdlg.h,v 1.2 2011-03-23 12:00:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "seistype.h"
class uiIOObjSel;
class uiGenInput;
class uiFileInput;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mClass uiResortSEGYDlg : public uiDialog
{
public :

			uiResortSEGYDlg(uiParent*);

    Seis::GeomType	geomType() const;

protected:

    uiGenInput*		geomfld_;
    uiIOObjSel*		volfld_;
    uiIOObjSel*		ps3dfld_;
    uiIOObjSel*		ps2dfld_;
    uiGenInput*		linesfld_;
    uiFileInput*	outfld_;

    uiIOObjSel*		objSel();

    void		geomSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
