#ifndef uihor3dfrom2ddlg_h
#define uihor3dfrom2ddlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.h,v 1.3 2007-01-29 18:42:12 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiCheckBox;
class CtxtIOObj;
namespace EM { class Horizon2D; };


/*! \brief Dialog to expand a 2D horizon to create a 3D horizon */

class uiHor3DFrom2DDlg : public uiDialog
{
public:    
				uiHor3DFrom2DDlg(uiParent*,
						 const EM::Horizon2D&);
				~uiHor3DFrom2DDlg();

protected:

    bool			acceptOK(CallBacker*);

    CtxtIOObj&			ctio_;
    const EM::Horizon2D& 	hor2d_;

    uiGenInput*			nriterfld_;
    uiIOObjSel*			outfld_;
    uiCheckBox*			displayfld_;

};

#endif
