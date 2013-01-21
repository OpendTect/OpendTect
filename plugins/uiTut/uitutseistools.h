#ifndef uitutseistools_h
#define uitutseistools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id$
-*/

#include "uitutmod.h"
#include "uidialog.h"
#include "seistype.h"
class uiSeisSel;
class uiSeisSubSel;
class uiGenInput;
class CtxtIOObj;
namespace Tut { class SeisTools; }


mExpClass(uiTut) uiTutSeisTools : public uiDialog
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
    uiGenInput*		newsdfld_;

    bool		acceptOK(CallBacker*);
    void		choiceSel(CallBacker*);
    void		inpSel(CallBacker*);
    void		doProc(CallBacker*);

};


#endif
