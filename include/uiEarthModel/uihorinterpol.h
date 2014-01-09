#ifndef uihorinterpol_h
#define uihorinterpol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "factory.h"
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
{
public:
    			uiHorizonInterpolDlg(uiParent*,EM::Horizon*,
					     bool is2d=false);
			~uiHorizonInterpolDlg();

    const char*		helpID() const;
    uiHorSaveFieldGrp*	saveFldGrp() const { return savefldgrp_; }

    Notifier<uiHorizonInterpolDlg> finished;

protected:

    bool			acceptOK(CallBacker*);

    bool			interpolate3D(const IOPar&);
    bool			interpolate2D();
    bool			is2d_;
    uiIOObjSel*			inputhorsel_;
    uiHor3DInterpolSel*		interpolhor3dsel_;
    uiArray1DInterpolSel*	interpol1dsel_;
    uiHorSaveFieldGrp*          savefldgrp_;

    EM::Horizon*		horizon_;
};


mExpClass(uiEarthModel) uiHor3DInterpolSel : public uiGroup
{
public:
				uiHor3DInterpolSel(uiParent*,
						   bool musthandlefaults);
				~uiHor3DInterpolSel() {}

    BinID			getStep() const;
    void			setStep(const BinID&);

    bool			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

    void			methodSelCB(CallBacker*);

    uiGenInput*			filltypefld_;
    uiGenInput*			maxholeszfld_;
    uiGenInput*			stepfld_;
    uiGenInput*			methodsel_;

    ObjectSet<uiHor3DInterpol>	methodgrps_;
};


mExpClass(uiEarthModel) uiHor3DInterpol : public uiGroup
{
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
{
public:
				uiInvDistHor3DInterpol(uiParent*);

    static void			initClass();
    static uiHor3DInterpol*	create(uiParent*);
    static const char*		sFactoryKeyword() { return "Inverse Distance"; }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual bool		canHandleFaults() const { return true; }

protected:

    void			useRadiusCB(CallBacker*);
    void			doParamDlg(CallBacker*);

    uiGenInput*			radiusfld_;
    uiButton*			parbut_;
    uiFaultParSel*		fltselfld_;

    bool			cornersfirst_;
    int				stepsz_;
    int				nrsteps_;
};


mExpClass(uiEarthModel) uiTriangulationHor3DInterpol : public uiHor3DInterpol
{
public:
				uiTriangulationHor3DInterpol(uiParent*);

    static void			initClass();
    static uiHor3DInterpol*	create(uiParent*);
    static const char*		sFactoryKeyword() { return "Triangulation"; }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual bool		canHandleFaults() const { return true; }

protected:

    void			useNeighborCB(CallBacker*);
    
    uiGenInput*			maxdistfld_;
    uiCheckBox*			useneighborfld_;
    uiFaultParSel*		fltselfld_;
};


mExpClass(uiEarthModel) uiExtensionHor3DInterpol : public uiHor3DInterpol
{
public:
				uiExtensionHor3DInterpol(uiParent*);

    static void			initClass();
    static uiHor3DInterpol*	create(uiParent*);
    static const char*		sFactoryKeyword()	{ return "Extension"; }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual bool		canHandleFaults() const { return false; }

protected:

    void			useNeighborCB(CallBacker*);

    uiGenInput*			nrstepsfld_;

};



#endif

