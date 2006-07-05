#ifndef uiattrtrcselout_h
#define uiattrtrcselout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: uiattrtrcselout.h,v 1.6 2006-07-05 15:27:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "attribdescid.h"

class CtxtIOObj;
class IOPar;
class MultiID;
class NLAModel;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiBinIDSubSel;
class HorSampling;

namespace Attrib { class DescSet; }

/*! \brief
Trace Selection Output Batch dialog.
Used for calculating attributes between surfaces or withing a user-defined 
interval around a surface
*/


class uiAttrTrcSelOut : public uiFullBatchDialog
{
public:
    			uiAttrTrcSelOut(uiParent*,const Attrib::DescSet&,
				      const NLAModel*,const MultiID&,bool);
			~uiAttrTrcSelOut();
    void		getComputableSurf(HorSampling&);

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    bool		addNLA(Attrib::DescID&);
    void                objSel(CallBacker*);
    void                attrSel(CallBacker*);
    void		interpSel(CallBacker*);
    void		extraWidthSel(CallBacker*);
    void                cubeBoundsSel(CallBacker*);

    void		createSingleHorUI();
    void		createTwoHorUI();
    void		createAttrFld(uiGroup*);
    void		createZIntervalFld(uiGroup*);
    void		createExtraZTopFld(uiGroup*);
    void		createExtraZBotFld(uiGroup*);
    void		createSubSelFld(uiGroup*);
    void		createOutsideValFld(uiGroup*);
    void		createInterpFld(uiGroup*);
    void		createNrSampFld(uiGroup*);
    void		createAddWidthFld(uiGroup*);
    void		createWidthFld(uiGroup*);
    void		createMainHorFld(uiGroup*);
    void		createCubeBoundsFlds(uiGroup*);
    void		createOutputFld(uiGroup*);

    BufferString	createAddWidthLabel();

    CtxtIOObj&		ctio_;
    CtxtIOObj&		ctio2_;
    CtxtIOObj&		ctioout_;
    Attrib::DescSet&	ads_;
    const MultiID&	nlaid_;
    const NLAModel*	nlamodel_;

    uiAttrSel*		attrfld_;
    uiIOObjSel*		objfld_;
    uiIOObjSel*		obj2fld_;
    uiGenInput*		gatefld_;
    uiGenInput*         extraztopfld_;
    uiGenInput*         extrazbotfld_;
    uiBinIDSubSel*	subselfld_;
    uiGenInput*		outsidevalfld_;
    uiGenInput*		interpfld_;
    uiGenInput*		nrsampfld_;
    uiGenInput*		mainhorfld_;
    uiGenInput*		widthfld_;
    uiGenInput*		addwidthfld_;
    uiGenInput*		setcubeboundsfld_;
    uiGenInput*		cubeboundsfld_;
    uiSeisSel*          outpfld_;
    bool		usesinglehor_;
};

#endif
