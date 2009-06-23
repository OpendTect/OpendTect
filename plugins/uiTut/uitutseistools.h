#ifndef uitutseistools_h
#define uitutseistools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uitutseistools.h,v 1.8 2009-06-23 05:17:25 cvsraman Exp $
-*/

#include "uidialog.h"
#include "seistype.h"
class uiSeisSel;
class uiSeisSubSel;
class uiGenInput;
class CtxtIOObj;
namespace Tut { class SeisTools; }


class uiTutSeisTools : public uiDialog
{
public:

    			uiTutSeisTools(uiParent*,Seis::GeomType);
			~uiTutSeisTools();

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;
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

    bool		acceptOK(CallBacker*);
    void		choiceSel(CallBacker*);
    void		inpSel(CallBacker*);
    void		doProc(CallBacker*);

};


#endif
