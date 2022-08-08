#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiattremout.h"

class CtxtIOObj;
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

    CtxtIOObj&		ctio_;
    CtxtIOObj&		ctio2_;

    uiIOObjSel*		objfld_;
    uiIOObjSel*		obj2fld_;
    uiGenInput*		gatefld_;
    uiGenInput*		extraztopfld_;
    uiGenInput*		extrazbotfld_;
    uiSeisSubSel*	seissubselfld_;
    uiGenInput*		outsidevalfld_;
    uiGenInput*		interpfld_;
    uiGenInput*		nrsampfld_;
    uiGenInput*		mainhorfld_;
    uiGenInput*		widthfld_;
    uiGenInput*		addwidthfld_;
    uiGenInput*		setcubeboundsfld_;
    uiGenInput*		cubeboundsfld_;
    uiSeisSel*		outpfld_;
    uiDialog*		xparsdlg_;
    bool		usesinglehor_;

private:

    CtxtIOObj&		mkCtxtIOObjHor(bool);

};

