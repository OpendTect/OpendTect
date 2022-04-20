#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
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
    const EM::Horizon3D*	getHorizon() const override	{ return hor_; }

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

    const Pick::Set*		getPickSet() const override	{ return ps_; }
    const EM::Horizon3D*	getHorizon() const override	{ return hor_; }

    void			psSel(CallBacker*);
};
