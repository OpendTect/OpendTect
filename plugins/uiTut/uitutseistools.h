#ifndef uitutseistools_h
#define uitutseistools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uitutseistools.h,v 1.4 2007-05-09 15:58:49 cvsbert Exp $
-*/

#include "uidialog.h"
class uiSeisSel;
class uiGenInput;
class CtxtIOObj;
class SeisSingleTraceProc;
namespace Tut { class SeisTools; }


class uiTutSeisTools : public uiDialog
{
public:

    			uiTutSeisTools(uiParent*);
			~uiTutSeisTools();

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;
    Tut::SeisTools&	tst_;
    SeisSingleTraceProc* stp_;

    uiSeisSel*		inpfld_;
    uiSeisSel*		outfld_;
    uiGroup*		scalegrp_;
    uiGenInput*		choicefld_;
    uiGenInput*		factorfld_;
    uiGenInput*		shiftfld_;
    uiGenInput*		smoothszfld_;

    bool		acceptOK(CallBacker*);
    void		choiceSel(CallBacker*);
    void		doProc(CallBacker*);

};


#endif
