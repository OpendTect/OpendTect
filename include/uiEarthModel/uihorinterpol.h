#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2009
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "factory.h"
#include "polygon.h"
#include "uidialog.h"
#include "uigroup.h"

class uiCheckBox;
class uiGenInput;
namespace EM { class Horizon; }

class uiFaultParSel;
class uiHorSaveFieldGrp;
class uiArray1DInterpolSel;
class uiIOObjSel;
class uiHor3DInterpol;
class uiHor3DInterpolSel;


mExpClass(uiEarthModel) uiHorizonInterpolDlg : public uiDialog
{ mODTextTranslationClass(uiHorizonInterpolDlg);
public:
			uiHorizonInterpolDlg(uiParent*,EM::Horizon*,bool is2d);
			~uiHorizonInterpolDlg();

    uiHorSaveFieldGrp*	saveFldGrp() const { return savefldgrp_; }

    Notifier<uiHorizonInterpolDlg> finished;
    Notifier<uiHorizonInterpolDlg> horReadyForDisplay;

protected:

    bool			acceptOK(CallBacker*) override;

    bool			interpolate3D(const IOPar&);
    bool			interpolate2D();
    void			selChangeCB(CallBacker*);

    bool			is2d_;
    uiIOObjSel*			inputhorsel_;
    uiHor3DInterpolSel*		interpolhor3dsel_;
    uiArray1DInterpolSel*	interpol1dsel_;
    uiHorSaveFieldGrp*		savefldgrp_;

    EM::Horizon*		horizon_;
};


mExpClass(uiEarthModel) uiHor3DInterpolSel : public uiGroup
{ mODTextTranslationClass(uiHor3DInterpolSel);
public:
				uiHor3DInterpolSel(uiParent*,
						   bool musthandlefaults);
				~uiHor3DInterpolSel() {}

    BinID			getStep() const;
    void			setStep(const BinID&);

    bool			isFullSurvey() const;
    bool			isPolygon() const;
    bool			cropPolygon() const;
    bool			getPolygonRange(Interval<int>& inlrg,
						Interval<int>& crlrg);
    bool			readPolygon(ODPolygon<float>& poly) const;

    bool			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

    void			methodSelCB(CallBacker*);
    void			scopeChgCB(CallBacker*);

    uiGenInput*			filltypefld_;
    uiGenInput*			maxholeszfld_;
    uiGenInput*			stepfld_;
    uiGenInput*			methodsel_;
    uiIOObjSel*			polyfld_;

    ObjectSet<uiHor3DInterpol>	methodgrps_;
};


mExpClass(uiEarthModel) uiHor3DInterpol : public uiGroup
{ mODTextTranslationClass(uiHor3DInterpol);
public:
    mDefineFactory1ParamInClass(uiHor3DInterpol,uiParent*,factory);

    virtual			~uiHor3DInterpol()	{}

    virtual bool		fillPar(IOPar&) const	{ return false; }
    virtual bool		usePar(const IOPar&)	{ return false; }

    virtual bool		canHandleFaults() const { return false; }

protected:
				uiHor3DInterpol(uiParent*);
};


mExpClass(uiEarthModel) uiInvDistHor3DInterpol : public uiHor3DInterpol
{ mODTextTranslationClass(uiInvDistHor3DInterpol);
public:
				uiInvDistHor3DInterpol(uiParent*);

    static void			initClass();
    static uiHor3DInterpol*	create(uiParent*);

    bool			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			canHandleFaults() const override
				{ return true; }

    const char*			factoryKeyword() const override;

protected:

    void			doParamDlg(CallBacker*);

    uiGenInput*			radiusfld_;
    uiButton*			parbut_;
    uiFaultParSel*		fltselfld_;

    bool			cornersfirst_;
    int				stepsz_;
    int				nrsteps_;
};


mExpClass(uiEarthModel) uiTriangulationHor3DInterpol : public uiHor3DInterpol
{ mODTextTranslationClass(uiTriangulationHor3DInterpol);
public:
				uiTriangulationHor3DInterpol(uiParent*);

    static void			initClass();
    static uiHor3DInterpol*	create(uiParent*);

    bool			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			canHandleFaults() const override
				{ return true; }

    const char*			factoryKeyword() const override;

protected:

    void			useNeighborCB(CallBacker*);

    uiGenInput*			maxdistfld_;
    uiCheckBox*			useneighborfld_;
    uiFaultParSel*		fltselfld_;
};


mExpClass(uiEarthModel) uiExtensionHor3DInterpol : public uiHor3DInterpol
{ mODTextTranslationClass(uiExtensionHor3DInterpol);
public:
				uiExtensionHor3DInterpol(uiParent*);

    static void			initClass();
    static uiHor3DInterpol*	create(uiParent*);

    bool			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			canHandleFaults() const override
				{ return false; }

    const char*			factoryKeyword() const override;

protected:

    void			useNeighborCB(CallBacker*);

    uiGenInput*			nrstepsfld_;

};


mExpClass(uiEarthModel)uiContinuousCurvatureHor3DInterpol:public uiHor3DInterpol
{mODTextTranslationClass(uiContinuousCurvatureHor3DInterpol);
public:
    uiContinuousCurvatureHor3DInterpol(uiParent*);

    static void			initClass();
    static uiHor3DInterpol*	create(uiParent*);

    bool			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			canHandleFaults() const override
				{ return true; }

    const char*			factoryKeyword() const override;

    uiGenInput*			tensionfld_;
    uiGenInput*			radiusfld_;
    uiButton*			parbut_;

protected:

};

