#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

// Because of RefMan
#include "emhorizon3d.h"
#include "pickset.h"

class uiCheckList;
class uiGenInput;
class uiHorizon3DSel;
class uiIOObjSel;
class uiUnitSel;


/*! \brief UI for calculation of volume at horizons */

mExpClass(uiEarthModel) uiCalcHorVol : public uiDialog
{
mODTextTranslationClass(uiCalcHorVol)
protected:
			uiCalcHorVol(uiParent*,const uiString&);
			~uiCalcHorVol();

    uiGroup*		mkStdGrp();

    uiCheckList*	optsfld_		= nullptr;
    uiGenInput*		velfld_			= nullptr;
    uiGenInput*		areafld_		= nullptr;
    uiUnitSel*		areaunitfld_		= nullptr;
    uiGenInput*		volumefld_;
    uiUnitSel*		volumeunitfld_;

    const bool		zinft_;
    float		areainm2_		= mUdf(float);
    float		volumeinm3_		= mUdf(float);

    void		unitChgCB(CallBacker*);
    void		haveChg(CallBacker*);
    void		calcReq(CallBacker*);

    virtual ConstRefMan<Pick::Set>	getPickSet() const		= 0;
    virtual ConstRefMan<EM::Horizon3D>	getHorizon() const		= 0;

};


/*! \brief using polygon to calculate to different horizons */

mExpClass(uiEarthModel) uiCalcPolyHorVol : public uiCalcHorVol
{
mODTextTranslationClass(uiCalcPolyHorVol)
public:

			uiCalcPolyHorVol(uiParent*,const Pick::Set&);
			~uiCalcPolyHorVol();

protected:

    uiHorizon3DSel*		horsel_		= nullptr;

    ConstRefMan<Pick::Set>	ps_;
    RefMan<EM::Horizon3D>	hor_;

    ConstRefMan<Pick::Set>	getPickSet() const override
				{ return ps_.ptr(); }
    ConstRefMan<EM::Horizon3D>	getHorizon() const override;

    void			horSel(CallBacker*);
};


/*! \brief using horizon to calculate from different levels by polygon */

mExpClass(uiEarthModel) uiCalcHorPolyVol : public uiCalcHorVol
{
mODTextTranslationClass(uiCalcHorPolyVol)
public:

			uiCalcHorPolyVol(uiParent*,const EM::Horizon3D&);
			~uiCalcHorPolyVol();

protected:

    uiIOObjSel*			pssel_		= nullptr;

    RefMan<Pick::Set>		ps_;
    ConstRefMan<EM::Horizon3D>	hor_;

    ConstRefMan<Pick::Set>	getPickSet() const override;
    ConstRefMan<EM::Horizon3D>	getHorizon() const override
				{ return hor_.ptr(); }

    void			psSel(CallBacker*);
};
