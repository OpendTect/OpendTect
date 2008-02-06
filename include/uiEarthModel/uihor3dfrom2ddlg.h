#ifndef uihor3dfrom2ddlg_h
#define uihor3dfrom2ddlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.h,v 1.5 2008-02-06 10:20:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "multiid.h"

#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiCheckBox;
class CtxtIOObj;
class uiEMPartServer;
namespace EM { class Horizon2D; };


/*! \brief Dialog to expand a 2D horizon to create a 3D horizon */

class uiHor3DFrom2DDlg : public uiDialog
{
public:    
				uiHor3DFrom2DDlg(uiParent*,
						 const EM::Horizon2D&,
						 uiEMPartServer*);
				~uiHor3DFrom2DDlg();

     bool			doDisplay() const;
     MultiID			getSelID() const;

protected:

    bool			acceptOK(CallBacker*);

    CtxtIOObj&			ctio_;
    const EM::Horizon2D& 	hor2d_;
    uiEMPartServer*		emserv_;

    uiGenInput*			grdtypfld_;
    uiGenInput*			nriterfld_;
    uiGenInput*			srchradfld_;
    uiIOObjSel*			outfld_;
    uiCheckBox*			displayfld_;

    MultiID			selid_;

    void			typChg(CallBacker*);

};

#endif
