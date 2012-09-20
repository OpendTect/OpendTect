#ifndef uistratamp_h
#define uistratamp_h

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nageswara
 * DATE     : Mar 2008
 * ID       : $Id$
-*/

#include "uibatchlaunch.h"

class CtxtIOObj;
class HorSampling;
class IOPar;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiLabeledComboBox;
class uiPosSubSel;


class uiStratAmpCalc : public uiFullBatchDialog
{
public:
			uiStratAmpCalc(uiParent*);
			~uiStratAmpCalc();
		      
protected:
    void		inpSel(CallBacker*);
    void		horSel(CallBacker*);
    void		choiceSel(CallBacker*);
    void		setParFileNameCB(CallBacker*);
    void		getAvailableRange(HorSampling&);
    bool		prepareProcessing();
    bool		checkInpFlds();
    bool		fillPar(IOPar& iop);
    void		setParFileName();

    CtxtIOObj&		horctio1_;
    CtxtIOObj&		horctio2_;

    uiGenInput*		winoption_;
    uiGenInput*		zoffset_;
    uiGenInput*		tophorshiftfld_;
    uiGenInput*		bothorshiftfld_;
    uiGenInput*		selfld_;
    uiGenInput*		foldfld_;
    uiGenInput*		attribnamefld_;
    uiAttrSel*		inpfld_;
    uiIOObjSel*		horfld1_;
    uiIOObjSel*		horfld2_;
    uiPosSubSel*	rangefld_;
    uiLabeledComboBox*	ampoptionfld_;
    bool		usesingle_;
    bool		isoverwrite_;
};


#endif
