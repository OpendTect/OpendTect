#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.2 2002-06-21 16:02:41 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiIOObjSel;
class uiFileInput;
class uiGenInput;
class CtxtIOObj;


class uiSeisImpCBVS : public uiDialog
{
public:

			uiSeisImpCBVS(uiParent*);
			~uiSeisImpCBVS();

protected:

    CtxtIOObj&		ctio_;

    uiFileInput*	inpfld;
    uiGenInput*		typefld;
    uiGenInput*		modefld;
    uiIOObjSel*		seissel;

    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
