#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "multiid.h"

#include "uidialog.h"

class uiArray2DInterpolSel;
class uiIOObjSel;
class uiCheckBox;
class uiEMPartServer;
namespace EM { class Horizon2D; class Horizon3D; }


/*! \brief Dialog to expand a 2D horizon to create a 3D horizon */

mExpClass(uiEarthModel) uiHor3DFrom2DDlg : public uiDialog
{ mODTextTranslationClass(uiHor3DFrom2DDlg);
public:
				uiHor3DFrom2DDlg(uiParent*,
						 const EM::Horizon2D&,
						 uiEMPartServer* emsrv=0);
				~uiHor3DFrom2DDlg();

    bool			doDisplay() const;
    MultiID			getSelID() const;
    EM::Horizon3D*		getHor3D();

protected:

    bool			acceptOK(CallBacker*);

    const EM::Horizon2D& 	hor2d_;
    EM::Horizon3D* 		hor3d_;
    uiEMPartServer*		emserv_;

    uiArray2DInterpolSel*	interpolsel_;

    uiIOObjSel*			outfld_;
    uiCheckBox*			displayfld_;

    MultiID			selid_;
};

