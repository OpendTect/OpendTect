#pragma once
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2009
 _______________________________________________________________________

      -*/


#include "uisegycommon.h"
#include "uiwellattribmod.h"
#include "uidialog.h"
#include "iopar.h"
#include "samplingdata.h"
#include "uistring.h"
class uiSEGYVSPBasicPars;
class uiGenInput;
class uiCheckBox;
class uiComboBox;
class uiIOObjSel;
class SeisTrc;
class CtxtIOObj;


mExpClass(uiSEGYTools) uiWellImportSEGYVSP : public uiDialog
{ mODTextTranslationClass(uiWellImportSEGYVSP);
public:
			uiWellImportSEGYVSP(uiParent*);
			~uiWellImportSEGYVSP();

    void		use(const SeisTrc&);

protected:

    uiGenInput*		inpsampfld_;
    uiGenInput*		istimefld_;
    uiGenInput*		unitfld_;
    uiGenInput*		outzrgfld_;
    uiCheckBox*		inpinftfld_;
    uiCheckBox*		outinftfld_;
    uiCheckBox*		inpistvdfld_;
    uiCheckBox*		outistvdfld_;
    uiComboBox*		lognmfld_;
    uiSEGYVSPBasicPars*	bparsfld_;
    uiIOObjSel*		wellfld_;

    IOPar		sgypars_;
    SamplingData<float>	dispinpsamp_;
    bool		isdpth_;

    bool		inpIsTime() const;
    void		isTimeChg(CallBacker*);
    void		wllSel(CallBacker*);
    void		outSampChk(CallBacker*);

    bool		acceptOK();
    bool		fetchTrc(SeisTrc&);
    bool		createLog(const SeisTrc&,const Interval<float>&,
				  const char*);

    friend class	uiSEGYVSPBasicPars;

};
