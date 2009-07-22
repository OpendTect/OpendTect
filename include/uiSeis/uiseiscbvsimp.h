#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.11 2009-07-22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiSeisSel;
class uiSeisTransfer;

/*!\brief Actually imports or just transfers data through selection */

mClass uiSeisImpCBVS : public uiDialog
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
    uiCheckBox*		convertfld;
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
