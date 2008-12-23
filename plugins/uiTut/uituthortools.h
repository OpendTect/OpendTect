#ifndef uituthortools_h
#define uituthortools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uituthortools.h,v 1.4 2008-12-23 13:53:37 cvsbert Exp $
-*/

#include "uidialog.h"
#include "tuthortools.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uicombobox.h"

class uiGenInput;
class CtxtIOObj;
namespace Tut { class HorTools; }


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

    void		choiceSel( CallBacker* );
    bool		acceptOK(CallBacker*);

};


#endif
