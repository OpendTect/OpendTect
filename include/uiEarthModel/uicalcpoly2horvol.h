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
class uiIOObjSel;


/*! \brief UI for calculation of volume at horizons */

mExpClass(uiEarthModel) uiCalcHorVol : public uiDialog
{
mODTextTranslationClass(uiCalcHorVol)
protected:
			uiCalcHorVol(uiParent*,const uiString&);
    virtual		~uiCalcHorVol();

    uiCheckList*	optsfld_	= nullptr;
    uiGenInput*		velfld_		= nullptr;
    uiGenInput*		valfld_		= nullptr;

    uiGroup*		mkStdGrp();

    const bool		zinft_;

    void		unitChgCB(CallBacker*);
    void		haveChg(CallBacker*);
    void		calcReq(CallBacker*);

    virtual const Pick::Set*		getPickSet() const		= 0;
    virtual const EM::Horizon3D*	getHorizon() const		= 0;

};


/*! \brief using polygon to calculate to different horizons */

mExpClass(uiEarthModel) uiCalcPolyHorVol : public uiCalcHorVol
{
mODTextTranslationClass(uiCalcPolyHorVol)
public:

			uiCalcPolyHorVol(uiParent*,const Pick::Set&);
			~uiCalcPolyHorVol();

protected:

    uiIOObjSel*			horsel_		= nullptr;

    ConstRefMan<Pick::Set>	ps_;
    RefMan<EM::Horizon3D>	hor_		= nullptr;

    const Pick::Set*		getPickSet() const override	{ return ps_; }
    const EM::Horizon3D*	getHorizon() const override
				{
				    if ( !hor_ )
				    {
					auto* nc =cCast(uiCalcPolyHorVol*,this);
					nc->horSel( nullptr);
				    }

				    return hor_;
				}

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

    RefMan<Pick::Set>		ps_		= nullptr;
    ConstRefMan<EM::Horizon3D>	hor_;

    const Pick::Set*		getPickSet() const override
				{
				    if ( !ps_ )
				    {
					auto* nc =cCast(uiCalcHorPolyVol*,this);
					nc->psSel( nullptr);
				    }
				    return ps_;
				}

    const EM::Horizon3D*	getHorizon() const override	{ return hor_; }

    void			psSel(CallBacker*);
};
