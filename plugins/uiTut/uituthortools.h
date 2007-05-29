#ifndef uituthortools_h
#define uituthortools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uituthortools.h,v 1.1 2007-05-29 06:37:19 cvsraman Exp $
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

    void		fillParThickness();
    void                fillParFilter();
    void		saveData(bool);
    EM::Horizon3D*	loadHor(IOObj*);

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&          inctio2_;
    CtxtIOObj&		outctio_;
    Tut::HorTools*	tst_;
    Tut::HorFilter*	ftr_;

    uiGenInput*		taskfld_;
    uiIOObjSel*		inpfld_;
    uiIOObjSel*         inpfld2_;
    uiGenInput*		selfld_;
    uiIOObjSel*		outfld_;

    void		choiceSel( CallBacker* );
    bool		acceptOK(CallBacker*);

};


#endif
