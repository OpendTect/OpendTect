#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.13 2009-09-15 09:46:51 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiScaler;
class uiSeisSel;
class uiSeisTransfer;
class uiSelection2DParSel;
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


mClass uiSeisCopyLineSet : public uiDialog
{
public:

			uiSeisCopyLineSet(uiParent*,const IOObj*);
			~uiSeisCopyLineSet();

protected:

    CtxtIOObj&		outctio_;

    uiSelection2DParSel*	inpfld_;

    uiScaler*		scalefld_;
    uiSeisSel*		outpfld_;

    bool		acceptOK(CallBacker*);

};


#endif
