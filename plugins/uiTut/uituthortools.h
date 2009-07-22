#ifndef uituthortools_h
#define uituthortools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : Mar 2007
 * ID       : $Id: uituthortools.h,v 1.7 2009-07-22 16:01:28 cvsbert Exp $
-*/

#include "uidialog.h"

class IOObj;
class CtxtIOObj;
class uiIOObjSel;
class uiGenInput;
class uiTaskRunner;

namespace Tut { class HorSmoother; class ThicknessCalculator; }
namespace EM { class Horizon3D; }


class uiTutHorTools : public uiDialog
{
public:

    			uiTutHorTools(uiParent*);
			~uiTutHorTools();

protected:

    bool		initThicknessCalculator(uiTaskRunner*);
    bool                initHorSmoother(uiTaskRunner*);
    void		saveData(bool);
    EM::Horizon3D*	loadHor(const IOObj&);

    CtxtIOObj&		inctio_;
    CtxtIOObj&          inctio2_;
    CtxtIOObj&		outctio_;

    Tut::ThicknessCalculator*	thickcalc_;
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
