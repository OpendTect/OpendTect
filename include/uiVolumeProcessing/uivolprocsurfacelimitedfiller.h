#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
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

mClass(uiVolumeProcessing) uiSurfaceLimitedFiller : public uiStepDialog
{ mODTextTranslationClass(uiSurfaceLimitedFiller)
public:
    mDefaultFactoryInstanciationBase(
	VolProc::SurfaceLimitedFiller::sFactoryKeyword(),
	VolProc::SurfaceLimitedFiller::sFactoryDisplayName())
	mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )

protected:

				uiSurfaceLimitedFiller(uiParent*,
					SurfaceLimitedFiller*,bool is2d);
				~uiSurfaceLimitedFiller();

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

    uiTable*			table_;
    uiPushButton*		addbutton_;
    uiPushButton*		removebutton_;

    uiGenInput*			usestartvalfld_;
    uiGenInput*			startvalfld_;
    uiHorizonAuxDataSel*	startgridfld_;

    uiGenInput*			usegradientfld_;
    uiGenInput*			gradientfld_;
    uiGenInput*			gradienttypefld_;
    uiHorizonAuxDataSel*	gradgridfld_;

    uiGenInput*			userefdepthfld_;
    uiGenInput*			refdepthfld_;
    uiIOObjSel*			refhorizonfld_;
};

} // namespace VolProc

