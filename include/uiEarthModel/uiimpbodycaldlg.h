#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiGenInput;
class uiUnitSel;
namespace EM { class Body; class ImplicitBody; }


/*! \brief UI for volume calculation of implicit body */

mExpClass(uiEarthModel) uiImplBodyCalDlg : public uiDialog
{
mODTextTranslationClass(uiImplBodyCalDlg);
public:
			uiImplBodyCalDlg(uiParent*,const EM::Body&);
			~uiImplBodyCalDlg();

    double		getVolume() const;

    uiObject*		attachObject();

    static Notifier<uiImplBodyCalDlg>&	instanceCreated();
    Notifier<uiImplBodyCalDlg>	calcPushed;

protected:
    
    void		calcCB(CallBacker*);
    void		unitChgCB(CallBacker*);
    void		getImpBody();

    const EM::Body&	embody_;
    EM::ImplicitBody*	impbody_		= nullptr;
    uiUnitSel*		unitfld_;
    uiGenInput*		velfld_			= nullptr;
    uiGenInput*		grossvolfld_;
    double		volumeinm3_		= mUdf(double);
    uiGroup*		topgrp_;
};
