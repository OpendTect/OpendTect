#ifndef uistratamp_h
#define uistratamp_h

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nageswara
 * DATE     : Mar 2008
 * ID       : $Id: uistratamp.h,v 1.6 2011-03-03 13:32:12 cvshelene Exp $
-*/

#include "uidialog.h"

class CtxtIOObj;
class HorSampling;
class IOObj;
class IOPar;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiLabeledComboBox;
class uiPosSubSel;

namespace EM { class Horizon3D; }

class uiStratAmpCalc : public uiDialog
{
public:
			uiStratAmpCalc(uiParent*);
			~uiStratAmpCalc();
		      
protected:

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

    bool		checkInpFlds();
    void		getAvailableRange(HorSampling&);
    bool		saveData(const EM::Horizon3D*,int,bool);
    EM::Horizon3D*	loadHor(const IOObj*,const HorSampling&);
    void		fillInEngineIOPar(IOPar&,const HorSampling&) const;

    void		inpSel(CallBacker*);
    void		horSel(CallBacker*);
    void		choiceSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
