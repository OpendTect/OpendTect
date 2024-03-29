#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiattremout.h"

class TrcKeySampling;
class NLAModel;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSeisSubSel;

namespace Attrib { class DescSet; }

/*! \brief
Trace Selection Output Batch dialog.
Used for calculating attributes between surfaces or withing a user-defined
interval around a surface
*/


mExpClass(uiEMAttrib) uiAttrTrcSelOut : public uiAttrEMOut
{ mODTextTranslationClass(uiAttrTrcSelOut)
public:
			uiAttrTrcSelOut(uiParent*,const Attrib::DescSet&,
				      const NLAModel*,const MultiID&,bool);
			~uiAttrTrcSelOut();

    void		getComputableSurf(TrcKeySampling&);

protected:

    bool		prepareProcessing() override;
    bool		fillPar(IOPar&) override;
    void		objSel(CallBacker*);
    void		attribSel(CallBacker*) override;
    void		interpSel(CallBacker*);
    void		extraWidthSel(CallBacker*);
    void		cubeBoundsSel(CallBacker*);
    void		extraParsCB(CallBacker*);
    void		extraDlgDone(CallBacker*);
    void		lineSel(CallBacker*);
    void		undefCB(CallBacker*);

    void		createSingleHorUI();
    void		createTwoHorUI();
    void		createZIntervalFld(uiParent*);
    void		createExtraZTopFld(uiParent*);
    void		createExtraZBotFld(uiParent*);
    void		createSubSelFld(uiParent*);
    void		createOutsideValFld(uiParent*);
    void		createInterpFld(uiParent*);
    void		createNrSampFld(uiParent*);
    void		createAddWidthFld(uiParent*);
    void		createWidthFld(uiParent*);
    void		createMainHorFld(uiParent*);
    void		createCubeBoundsFlds(uiParent*);
    void		createOutputFld(uiParent*);
    void		getJobName(BufferString&) const override;

    uiString		createAddWidthLabel();

    uiIOObjSel*		objfld_;
    uiIOObjSel*		obj2fld_		= nullptr;
    uiGenInput*		gatefld_		= nullptr;
    uiGenInput*		extraztopfld_		= nullptr;
    uiGenInput*		extrazbotfld_		= nullptr;
    uiSeisSubSel*	seissubselfld_;
    uiGenInput*		outsidevalfld_;
    uiGenInput*		interpfld_		= nullptr;
    uiGenInput*		nrsampfld_		= nullptr;
    uiGenInput*		mainhorfld_		= nullptr;
    uiGenInput*		widthfld_		= nullptr;
    uiGenInput*		addwidthfld_		= nullptr;
    uiGenInput*		setcubeboundsfld_;
    uiGenInput*		cubeboundsfld_;
    uiSeisSel*		outpfld_;
    uiDialog*		xparsdlg_		= nullptr;
    bool		usesinglehor_;
    bool		is2d_;
};
