#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.12 2009-08-04 12:24:41 cvsbert Exp $
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
class uiLabeledComboBox;

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
    uiLabeledComboBox*	compfld_;

    void		finpSel(CallBacker*);
    void		oinpSel(CallBacker*);
    void		modeSel(CallBacker*);
    void		typeChg(CallBacker*);

    IOObj*		getfInpIOObj(const char*) const;

    bool		acceptOK(CallBacker*);

    bool		ismc_;

private:

    void		init(bool);

};


#endif
