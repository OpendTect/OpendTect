#ifndef uiattrtrcselout_h
#define uiattrtrcselout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: uiattrtrcselout.h,v 1.5 2006-02-22 12:32:09 cvshelene Exp $
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
    void		createOutputFld(uiGroup*);

    BufferString	createAddWidthLabel();

    CtxtIOObj&		ctio;
    CtxtIOObj&		ctio2;
    CtxtIOObj&		ctioout;
    Attrib::DescSet&	ads;
    const MultiID&	nlaid;
    const NLAModel*	nlamodel;

    uiAttrSel*		attrfld;
    uiIOObjSel*		objfld;
    uiIOObjSel*		obj2fld;
    uiGenInput*		gatefld;
    uiGenInput*         extraztopfld;
    uiGenInput*         extrazbotfld;
    uiBinIDSubSel*	subselfld;
    uiGenInput*		outsidevalfld;
    uiGenInput*		interpfld;
    uiGenInput*		nrsampfld;
    uiGenInput*		mainhorfld;
    uiGenInput*		widthfld;
    uiGenInput*		addwidthfld;
    uiSeisSel*          outpfld;
    bool		usesinglehor_;
};

#endif
