#ifndef uiattrtrcselout_h
#define uiattrtrcselout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattremout.h"

class CtxtIOObj;
class IOPar;
class MultiID;
class NLAModel;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSeisSubSel;
class HorSampling;

namespace Attrib { class DescSet; }

/*! \brief
Trace Selection Output Batch dialog.
Used for calculating attributes between surfaces or withing a user-defined 
interval around a surface
*/


mClass uiAttrTrcSelOut : public uiAttrEMOut
{
public:
    			uiAttrTrcSelOut(uiParent*,const Attrib::DescSet&,
				      const NLAModel*,const MultiID&,bool);
			~uiAttrTrcSelOut();
    void		getComputableSurf(HorSampling&);

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    void                objSel(CallBacker*);
    void                attribSel(CallBacker*);
    void		interpSel(CallBacker*);
    void		extraWidthSel(CallBacker*);
    void                cubeBoundsSel(CallBacker*);
    void                extraParsCB(CallBacker*);
    void                extraDlgDone(CallBacker*);
    void                singLineSel(CallBacker*);

    void		createSingleHorUI();
    void		createTwoHorUI();
    void		createAttrFld(uiParent*);
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

    BufferString	createAddWidthLabel();

    CtxtIOObj&		ctio_;
    CtxtIOObj&		ctio2_;
    CtxtIOObj&		ctioout_;

    uiIOObjSel*		objfld_;
    uiIOObjSel*		obj2fld_;
    uiGenInput*		gatefld_;
    uiGenInput*         extraztopfld_;
    uiGenInput*         extrazbotfld_;
    uiSeisSubSel*	seissubselfld_;
    uiGenInput*		outsidevalfld_;
    uiGenInput*		interpfld_;
    uiGenInput*		nrsampfld_;
    uiGenInput*		mainhorfld_;
    uiGenInput*		widthfld_;
    uiGenInput*		addwidthfld_;
    uiGenInput*		setcubeboundsfld_;
    uiGenInput*		cubeboundsfld_;
    uiSeisSel*          outpfld_;
    uiDialog*		xparsdlg_;
    bool		usesinglehor_;

private:

    CtxtIOObj&          mkCtxtIOObjHor(bool);
};

#endif
