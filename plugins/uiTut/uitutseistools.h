#ifndef uitutseistools_h
#define uitutseistools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: uitutseistools.h,v 1.11 2012-08-07 04:23:04 cvsmahant Exp $
-*/

#include "uidialog.h"
#include "seistype.h"
#include "uibutton.h" //Modified

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
    uiGenInput*		newsdfld_;
    


    // Modified
    uiPushButton*	resetbut_; //Modified
    //    uiRadioButton*	resetbut1_; //Mod..

    void           	selectReset(CallBacker*); //Modified
    
    //




    bool		acceptOK(CallBacker*);
    void		choiceSel(CallBacker*);
    void		inpSel(CallBacker*);
    void		doProc(CallBacker*);

};


#endif
