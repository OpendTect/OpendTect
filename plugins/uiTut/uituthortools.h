#ifndef uituthortools_h
#define uituthortools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uituthortools.h,v 1.5 2009-04-09 11:49:08 cvsranojay Exp $
-*/

#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class CtxtIOObj;
class IOObj;

namespace Tut { class HorSmoother; class ThicknessFinder; }
namespace EM { class Horizon3D; }


class uiTutHorTools : public uiDialog
{
public:

    			uiTutHorTools(uiParent*);
			~uiTutHorTools();

protected:

    bool		initThicknessFinder();
    bool                initHorSmoother();
    void		saveData(bool);
    EM::Horizon3D*	loadHor(const IOObj*);

    CtxtIOObj&		inctio_;
    CtxtIOObj&          inctio2_;
    CtxtIOObj&		outctio_;

    Tut::ThicknessFinder*	thickcalc_;
    Tut::HorSmoother*		smoother_;

    uiGenInput*		taskfld_;
    uiIOObjSel*		inpfld_;
    uiIOObjSel*         inpfld2_;
    uiGenInput*		selfld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		strengthfld_;
    uiGenInput*		attribnamefld_;

    void		choiceSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
