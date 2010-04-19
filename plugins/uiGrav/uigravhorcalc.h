#ifndef uigravhorcalc_h
#define uigravhorcalc_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Apr 2010
-*/

#include "uidialog.h"
#include "emposid.h"
class uiGenInput;
class uiIOObjSel;
class uiVelSel;


class uiGravHorCalc : public uiDialog
{
public:

			uiGravHorCalc(uiParent*,EM::ObjectID);

protected:

    const MultiID	horid_;
    BufferString	hornm_;

    uiIOObjSel*		topfld_;
    uiIOObjSel*		botfld_;
    uiVelSel*		velfld_;
    uiGenInput*		denvarfld_;
    uiGenInput*		denvaluefld_;
    uiGenInput*		denattrfld_;
    uiGenInput*		cutoffangfld_;
    uiGenInput*		attrnmfld_;

    void		initFlds(CallBacker*);
    void		denVarSel(CallBacker*);
    void		topSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};


#endif
