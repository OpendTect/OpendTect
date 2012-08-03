#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseiscbvsimp.h,v 1.17 2012-08-03 13:01:08 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class SeisSingleTraceProc;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiScaler;
class uiSeisSel;
class uiSeisTransfer;
class uiSeis2DMultiLineSel;
class uiLabeledComboBox;

/*!\brief Actually imports or just transfers data through selection */

mClass(uiSeis) uiSeisImpCBVS : public uiDialog
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
    void		convertSel(CallBacker*);

    IOObj*		getfInpIOObj(const char*) const;
    void		getOutputName(BufferString&) const;

    bool		acceptOK(CallBacker*);
    void		procToBeDoneCB(CallBacker*);

    SeisSingleTraceProc* sstp_;

    bool		ismc_;

private:

    void		init(bool);

};


mClass(uiSeis) uiSeisCopyLineSet : public uiDialog
{
public:

			uiSeisCopyLineSet(uiParent*,const IOObj*);
			~uiSeisCopyLineSet();

protected:

    CtxtIOObj&		outctio_;

    uiSeis2DMultiLineSel* inpfld_;
    uiScaler*		scalefld_;
    uiSeisSel*		outpfld_;

    bool		acceptOK(CallBacker*);

};


#endif

