#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.3 2002-06-21 22:37:04 bert Exp $
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
    uiIOObjSel*		seissel;

    void		inpSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};


#endif
