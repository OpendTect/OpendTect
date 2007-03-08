#ifndef uitutseistools_h
#define uitutseistools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uitutseistools.h,v 1.1 2007-03-08 15:33:09 cvsraman Exp $
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
    uiGenInput*		strengthfld_;

    bool		acceptOK(CallBacker*);
    void		doProc(CallBacker*);

};


#endif
