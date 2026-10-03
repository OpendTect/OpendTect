#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "ctxtioobj.h"
#include "uidialog.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"

class uiFileInput;
class uiGenInput;
class uiSurfaceRead;
class uiMultiSurfaceRead;
class uiUnitSel;
class uiPushButton;
namespace Coords { class uiCoordSystemSel; }

/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiExportHorizon : public uiDialog
{ mODTextTranslationClass(uiExportHorizon);
public:

    mExpClass(uiEarthModel) Setup
    {
    public:
			Setup(bool isbulk=false);
			~Setup();

	mDefSetupMembInit(bool,isbulk,false)
	mDefSetupMemb(uiIOObjSel::Setup,objsel)
	mDefSetupMemb(uiIOObjSelGrp::Setup,multisel)
	mDefSetupMemb(IOObjSelConstraints,constraints)
    };

			uiExportHorizon(uiParent*,const Setup&);
			uiExportHorizon(uiParent*,bool isbulk);
			~uiExportHorizon();

    bool		isBulk() const		{ return isbulk_; }

protected:

    uiSurfaceRead*	infld_				= nullptr;
    uiFileInput*	outfld_;
    uiGenInput*		headerfld_;
    uiGenInput*		typfld_;
    uiGenInput*		writezfld_;
    uiPushButton*	settingsbutt_;
    uiUnitSel*		unitsel_;
    uiGenInput*		udffld_;
    uiMultiSurfaceRead* multisurfdepthread_		= nullptr;
    uiMultiSurfaceRead* multisurftimeread_		= nullptr;
    Coords::uiCoordSystemSel* coordsysselfld_		= nullptr;
    uiGenInput*		horzdomypefld_			= nullptr;

    BufferString	gfname_;
    BufferString	gfcomment_;

    bool		acceptOK(CallBacker*) override;
    void		typChg(CallBacker*);
    void		addZChg(CallBacker*);
    void		attrSel(CallBacker*);
    void		settingsCB(CallBacker*);
    void		inpSel(CallBacker*);
    void		writeHeader(od_ostream&);
    void		zDomainTypeChg(CallBacker*);
    void		initGrpCB(CallBacker*);
    void		onPopupCB(CallBacker*);

    bool		writeAscii();
    bool		getInputMIDs(TypeSet<MultiID>&);

    bool		isbulk_;

    bool		exportToGF() const;
};
