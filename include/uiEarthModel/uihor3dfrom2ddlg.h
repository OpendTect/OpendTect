#ifndef uihor3dfrom2ddlg_h
#define uihor3dfrom2ddlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.h,v 1.7 2009-04-23 18:08:50 cvskris Exp $
________________________________________________________________________

-*/

#include "multiid.h"

#include "uidialog.h"

class uiArray2DInterpolSel;
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;
class uiEMPartServer;
namespace EM { class Horizon2D; class Horizon3D; };


/*! \brief Dialog to expand a 2D horizon to create a 3D horizon */

mClass uiHor3DFrom2DDlg : public uiDialog
{
public:    
				uiHor3DFrom2DDlg(uiParent*,
						 const EM::Horizon2D&,
						 uiEMPartServer*);
				~uiHor3DFrom2DDlg();

    bool			doDisplay() const;
    MultiID			getSelID() const;
    EM::Horizon3D* 		getHor3D() { return hor3d_; }

protected:

    bool			acceptOK(CallBacker*);


    const EM::Horizon2D& 	hor2d_;
    EM::Horizon3D* 		hor3d_;
    uiEMPartServer*		emserv_;

    uiArray2DInterpolSel*	interpolsel_;

    uiIOObjSel*			outfld_;
    uiCheckBox*			displayfld_;

    MultiID			selid_;

    void			typChg(CallBacker*);
};

#endif
