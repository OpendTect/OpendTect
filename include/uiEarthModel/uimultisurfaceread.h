#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiiosurface.h"
#include "uidialog.h"

class uiIOObjSelGrp;
class uiDialog;
namespace ZDomain { class Info; }

/*! \brief ui for multiple surface read */

mExpClass(uiEarthModel) uiMultiSurfaceRead : public uiIOSurface
{ mODTextTranslationClass(uiMultiSurfaceRead);
public:
			uiMultiSurfaceRead(uiParent*,const char* type,
				const ZDomain::Info*);
			~uiMultiSurfaceRead();

    uiIOObjSelGrp*	objselGrp()		{ return ioobjselgrp_;}

    void		getSurfaceIds(TypeSet<MultiID>&) const;
    void		getSurfaceSelection(EM::SurfaceIODataSelection&) const;

    Notifier<uiMultiSurfaceRead>	singleSurfaceSelected;

protected:

    uiIOObjSelGrp*	ioobjselgrp_;

    void                dClck(CallBacker*);
    void		selCB(CallBacker*);
};


mExpClass(uiEarthModel) uiMultiSurfaceReadDlg : public uiDialog
{ mODTextTranslationClass(uiMultiSurfaceReadDlg)
public:
			uiMultiSurfaceReadDlg(uiParent*,const char* type,
					const ZDomain::Info* zinf=nullptr);
			~uiMultiSurfaceReadDlg();

    uiMultiSurfaceRead*	iogrp()		{ return surfacefld_; }

protected:

    void		statusMsg(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiMultiSurfaceRead*	surfacefld_;
};
