#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.1 2002-04-24 15:11:49 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiLabeledSpinBox;
class uiGenInput;
class CubeSampling;


class uiSliceSel : public uiDialog
{
public:
				uiSliceSel(uiParent*,CubeSampling*);

protected:

    void			selChg(CallBacker*);
    bool			acceptOK(CallBacker*);

    uiLabeledSpinBox*           inlfld;
    uiLabeledSpinBox*           crlfld;
    uiLabeledSpinBox*           zfld;
    uiGenInput*                 inlrgfld;
    uiGenInput*                 crlrgfld;
    uiGenInput*                 zrgfld;

    int				slctyp;
    CubeSampling*		cs;    
};

#endif
