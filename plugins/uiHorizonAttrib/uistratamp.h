#ifndef uistratamp_h
#define uistratamp_h

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nageswara
 * DATE     : Mar 2008
 * ID       : $Id: uistratamp.h,v 1.4 2009-07-22 16:01:28 cvsbert Exp $
-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class HorSampling;
class uiLabeledComboBox;
class uiPosSubSel;
class uiIOObjSel;
class uiSeisSel;
class uiGenInput;

namespace EM { class Horizon3D; }

class uiStratAmpCalc : public uiDialog
{
public:
			uiStratAmpCalc(uiParent*);
			~uiStratAmpCalc();
		      
protected:

    CtxtIOObj&		seisctio_;
    CtxtIOObj&		horctio1_;
    CtxtIOObj&		horctio2_;

    uiGenInput*		winoption_;
    uiGenInput*		zoffset_;
    uiGenInput*		tophorshiftfld_;
    uiGenInput*		bothorshiftfld_;
    uiGenInput*		selfld_;
    uiGenInput*		attribnamefld_;

    uiSeisSel*		inpfld_;
    uiIOObjSel*		horfld1_;
    uiIOObjSel*		horfld2_;
    uiPosSubSel*	rangefld_;
    uiLabeledComboBox*	ampoptionfld_;

    bool		usesingle_;

    bool		checkInpFlds();
    void		getAvailableRange(HorSampling&);
    bool		saveData(const EM::Horizon3D*,int,bool);
    EM::Horizon3D*	loadHor(const IOObj*,const HorSampling&);

    void		inpSel(CallBacker*);
    void		horSel(CallBacker*);
    void		choiceSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
