#ifndef uigravhorcalc_h
#define uigravhorcalc_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Apr 2010
 * SVN      : $Id$
-*/

#include "uidialog.h"
#include "emposid.h"

class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiT2DConvSel;


mClass(uiGrav) uiGravHorCalc : public uiDialog
{
public:

			uiGravHorCalc(uiParent*,EM::ObjectID);
			~uiGravHorCalc();

protected:

    const MultiID	horid_;
    const IOObj*	horioobj_;

    uiIOObjSel*		topfld_;
    uiIOObjSel*		botfld_;
    uiGenInput*		denvarfld_;
    uiGenInput*		denvaluefld_;
    uiGenInput*		denattrfld_;
    uiGenInput*		cutoffangfld_;
    uiGenInput*		attrnmfld_;
    uiT2DConvSel*	t2dfld_;

    void		initFlds(CallBacker*);
    void		denVarSel(CallBacker*);
    void		topSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};


#endif
