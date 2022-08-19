#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutmod.h"
#include "uidialog.h"
#include "seistype.h"
class uiSeisSel;
class uiSeisSubSel;
class uiGenInput;
namespace Tut { class SeisTools; }


mExpClass(uiTut) uiTutSeisTools : public uiDialog
{ mODTextTranslationClass(uiTutSeisTools);
public:

			uiTutSeisTools(uiParent*,Seis::GeomType);
			~uiTutSeisTools();

protected:

    Seis::GeomType	geom_;
    Tut::SeisTools&	tst_;

    uiSeisSel*		inpfld_;
    uiSeisSubSel*	subselfld_;
    uiSeisSel*		outfld_;
    uiGroup*		scalegrp_;
    uiGenInput*		actionfld_;
    uiGenInput*		factorfld_;
    uiGenInput*		shiftfld_;
    uiGenInput*		smoothszfld_;
    uiGenInput*		newsdfld_;

    bool		acceptOK(CallBacker*);
    void		choiceSel(CallBacker*);
    void		inpSel(CallBacker*);
    void		doProc(CallBacker*);

};
