#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
-*/

#include "uitutmod.h"
#include "uidialog.h"
#include "multiid.h"
#include "welldata.h"

class uiGenInput;
class uiListBox;
class uiLabeledSpinBox;
namespace Tut { class LogTools; }
namespace Well { class Data; }


mExpClass(uiTut) uiTutWellTools : public uiDialog
{ mODTextTranslationClass(uiTutWellTools);
public:

    			uiTutWellTools(uiParent*,const MultiID& wellid);
			~uiTutWellTools();

protected:
    uiListBox*		inplogfld_;
    uiGenInput*		outplogfld_;
    uiLabeledSpinBox*	gatefld_;

    BufferString	inlognm_;
    BufferString        outlognm_;
    RefMan<Well::Data>	wd_;
    MultiID		wellid_;

    void		inpchg(CallBacker*);
    void		wellToBeDeleted(CallBacker*);
    bool		acceptOK(CallBacker*);

};


