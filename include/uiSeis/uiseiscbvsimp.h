#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.5 2003-05-22 11:10:27 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class IOObj;
class CtxtIOObj;
class uiIOObjSel;
class uiGenInput;
class uiFileInput;
class uiSeisTransfer;

/*!\brief Actually imports or just transfers data through selection */

class uiSeisImpCBVS : public uiDialog
{
public:

			uiSeisImpCBVS(uiParent*);		//!< From file
			uiSeisImpCBVS(uiParent*,const IOObj*);	//!< From entry
			~uiSeisImpCBVS();

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    uiFileInput*	finpfld;
    uiIOObjSel*		oinpfld;
    uiGenInput*		typefld;
    uiGenInput*		modefld;
    uiSeisTransfer*	transffld;
    uiIOObjSel*		seissel;

    void		finpSel(CallBacker*);
    void		oinpSel(CallBacker*);
    void		modeSel(CallBacker*);
    void		typeChg(CallBacker*);

    IOObj*		getfInpIOObj(const char*) const;

    bool		acceptOK(CallBacker*);

private:

    void		init(bool);

};


#endif
