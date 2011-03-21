#ifndef uisegyresortdlg_h
#define uisegyresortdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2011
 RCS:           $Id: uisegyresortdlg.h,v 1.1 2011-03-21 16:16:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "seistype.h"
class uiSeisSel;
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
    uiSeisSel*		volfld_;
    uiSeisSel*		ps3dfld_;
    uiSeisSel*		ps2dfld_;
    uiGenInput*		linesfld_;
    uiFileInput*	outfld_;

    uiSeisSel*		seisSel();

    void		geomSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
