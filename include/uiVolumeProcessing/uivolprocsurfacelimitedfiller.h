#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"

#include "volprocsurfacelimitedfiller.h"

class uiGenInput;
class uiIOObjSel;
class IOObj;
class uiPushButton;
class uiTable;
class uiHorizonAuxDataSel;

namespace VolProc
{

class SurfaceLimitedFiller;

mExpClass(uiVolumeProcessing) uiSurfaceLimitedFiller : public uiStepDialog
{ mODTextTranslationClass(uiSurfaceLimitedFiller)
public:
    mDefaultFactoryInstanciationBase(
	VolProc::SurfaceLimitedFiller::sFactoryKeyword(),
	VolProc::SurfaceLimitedFiller::sFactoryDisplayName())
	mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )

				~uiSurfaceLimitedFiller();

protected:

				uiSurfaceLimitedFiller(uiParent*,
					SurfaceLimitedFiller*,bool is2d);

    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);
    bool			acceptOK(CallBacker*) override;

    void			addSurfaceCB(CallBacker*);
    void			removeSurfaceCB(CallBacker*);
    void			addSurfaceTableEntry(const IOObj&,
						     bool isfault,char side);
				/*Current row==surfaces size */

    void			refDepthTypeChangeCB(CallBacker*);
    void			useStartValCB(CallBacker*);
    void			useGradientCB(CallBacker*);
    void			useRefValCB(CallBacker*);

    SurfaceLimitedFiller*	surfacefiller_;
    TypeSet<MultiID>		surfacelist_;

    uiTable*			table_				= nullptr;
    uiPushButton*		addbutton_			= nullptr;
    uiPushButton*		removebutton_			= nullptr;

    uiGenInput*			usestartvalfld_			= nullptr;
    uiGenInput*			startvalfld_			= nullptr;
    uiHorizonAuxDataSel*	startgridfld_			= nullptr;

    uiGenInput*			usegradientfld_			= nullptr;
    uiGenInput*			gradientfld_			= nullptr;
    uiGenInput*			gradienttypefld_		= nullptr;
    uiHorizonAuxDataSel*	gradgridfld_			= nullptr;

    uiGenInput*			userefdepthfld_			= nullptr;
    uiGenInput*			refdepthfld_			= nullptr;
    uiIOObjSel*			refhorizonfld_			= nullptr;
};

} // namespace VolProc
