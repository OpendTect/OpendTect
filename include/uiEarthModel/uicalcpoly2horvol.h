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
#include "pickset.h"

class uiConstantVel;
class uiIOObjSel;
class uiPickSetIOObjSel;
class uiGenInput;
class uiCheckList;
namespace EM	{ class Horizon3D; }


/*! \brief UI for calculation of volume at horizons */

mExpClass(uiEarthModel) uiCalcHorVol : public uiDialog
{ mODTextTranslationClass(uiCalcHorVol)
protected:

			uiCalcHorVol(uiParent*,const uiString&);

    uiCheckList*	optsfld_;
    uiConstantVel*	velfld_;
    uiGenInput*		valfld_;

    uiGroup*		mkStdGrp();

    const bool		zinft_;

    void		haveChg(CallBacker*);
    void		calcReq(CallBacker*);

    virtual const Pick::Set*		getPickSet()		= 0;
    virtual const EM::Horizon3D*	getHorizon()		= 0;

};


/*! \brief using polygon to calculate to different horizons */

mExpClass(uiEarthModel) uiCalcPolyHorVol : public uiCalcHorVol
{ mODTextTranslationClass(uiCalcPolyHorVol)
public:

			uiCalcPolyHorVol(uiParent*,const Pick::Set&);
			~uiCalcPolyHorVol();

    const Pick::Set&	pickSet()		{ return ps_; }

protected:

    uiIOObjSel*		horsel_;

    const Pick::Set&	ps_;
    EM::Horizon3D*	hor_;

    const Pick::Set*		getPickSet()	{ return &ps_; }
    const EM::Horizon3D*	getHorizon();

    void		horSel(CallBacker*);

};


/*! \brief using horizon to calculate from different levels by polygon */

mExpClass(uiEarthModel) uiCalcHorPolyVol : public uiCalcHorVol
{ mODTextTranslationClass(uiCalcHorPolyVol)
public:

			uiCalcHorPolyVol(uiParent*,const EM::Horizon3D&);
			~uiCalcHorPolyVol();

    const EM::Horizon3D& horizon()		{ return hor_; }

protected:

    uiPickSetIOObjSel*	pssel_;

    ConstRefMan<Pick::Set> ps_;
    const EM::Horizon3D& hor_;

    const Pick::Set*		getPickSet();
    const EM::Horizon3D*	getHorizon()	{ return &hor_; }

    void		psSel(CallBacker*);

};
