#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "multiid.h"

#include "uidialog.h"

class uiArray2DInterpolSel;
class uiIOObjSel;
class uiCheckBox;
class uiEMPartServer;


/*! \brief Dialog to expand a 2D horizon to create a 3D horizon */

mExpClass(uiEarthModel) uiHor3DFrom2DDlg : public uiDialog
{ mODTextTranslationClass(uiHor3DFrom2DDlg);
public:
				uiHor3DFrom2DDlg(uiParent*,
						 const EM::Horizon2D&,
						 uiEMPartServer* =nullptr);
				~uiHor3DFrom2DDlg();

    bool			doDisplay() const;
    MultiID			getSelID() const;
    ConstRefMan<EM::Horizon3D>	getHor3D() const;

protected:

    bool			acceptOK(CallBacker*) override;

    ConstRefMan<EM::Horizon2D>	hor2d_;
    RefMan<EM::Horizon3D>	hor3d_;
    uiEMPartServer*		emserv_;

    uiArray2DInterpolSel*	interpolsel_;

    uiIOObjSel*			outfld_;
    uiCheckBox*			displayfld_		    = nullptr;

    MultiID			selid_;
};
