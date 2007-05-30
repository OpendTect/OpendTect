#ifndef uituthortools_h
#define uituthortools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uituthortools.h,v 1.2 2007-05-30 07:11:26 cvsraman Exp $
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
    bool                initHorSmoothener();
    void		saveData(bool);
    EM::Horizon3D*	loadHor(const IOObj*);

    CtxtIOObj&		inctio_;
    CtxtIOObj&          inctio2_;
    CtxtIOObj&		outctio_;

    Tut::ThicknessFinder*	thickcalc_;
    Tut::HorSmoothener*		smoothnr_;

    uiGenInput*		taskfld_;
    uiIOObjSel*		inpfld_;
    uiIOObjSel*         inpfld2_;
    uiGenInput*		selfld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		attribnamefld_;

    void		choiceSel( CallBacker* );
    bool		acceptOK(CallBacker*);

};


#endif
