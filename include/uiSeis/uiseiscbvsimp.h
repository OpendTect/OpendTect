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
#include "uiseiscopy.h"

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

/*!\brief DEPRECATED: will disappear after 5.0. Use either uiSeisCopyCube or
  uiSeisImportCBVS instead. */


mExpClass(uiSeis) uiSeisImpCBVS : public uiDialog
{
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


#endif
