#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.8 2005-05-10 11:25:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class IOObj;
class CtxtIOObj;
class uiSeisSel;
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
    BufferString	tmpid_;

    uiGenInput*		typefld;
    uiGenInput*		modefld;
    uiSeisTransfer*	transffld;
    uiFileInput*	finpfld;
    uiSeisSel*		oinpfld;
    uiSeisSel*		outfld;

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
