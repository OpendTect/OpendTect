#ifndef uiseiscbvsimp_h
#define uiseiscbvsimp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

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

mExpClass(uiSeis) uiSeisImpCBVS : public uiDialog
{ mODTextTranslationClass(uiSeisImpCBVS);
public:

			uiSeisImpCBVS(uiParent*);		//!< From file
			uiSeisImpCBVS(uiParent*,const IOObj*);	//!< From entry
			~uiSeisImpCBVS();

protected:

    IOObj*		initialinpioobj_;
    IOObj*		outioobj_;
    BufferString	tmpid_;

    uiGenInput*		typefld_;
    uiGenInput*		modefld_;
    uiCheckBox*		convertfld_;
    uiSeisTransfer*	transffld_;
    uiFileInput*	finpfld_;
    uiSeisSel*		oinpfld_;
    uiSeisSel*		outfld_;
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


mExpClass(uiSeis) uiSeisCopyLineSet : public uiDialog
{
public:

			uiSeisCopyLineSet(uiParent*,const IOObj*);
protected:

    uiSeisSel*		inpfld_;
    uiSeis2DMultiLineSel* subselfld_;
    uiScaler*		scalefld_;
    uiSeisSel*		outpfld_;

    bool		acceptOK(CallBacker*);

};


#endif

