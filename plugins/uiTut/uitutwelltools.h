#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
    bool		acceptOK(CallBacker*) override;

};
