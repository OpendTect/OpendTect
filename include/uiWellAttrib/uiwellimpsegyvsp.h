#ifndef uiwellimpsegyvsp_h
#define uiwellimpsegyvsp_h
/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
 RCS:           $Id: uiwellimpsegyvsp.h,v 1.5 2009-01-14 17:09:40 cvsbert Exp $
 _______________________________________________________________________

      -*/


#include "uidialog.h"
#include "iopar.h"
#include "samplingdata.h"
#include "bufstringset.h"
class uiSEGYVSPBasicPars;
class uiGenInput;
class uiCheckBox;
class uiIOObjSel;
class SeisTrc;
class CtxtIOObj;


mClass uiWellImportSEGYVSP : public uiDialog
{
public:
    			uiWellImportSEGYVSP(uiParent*);
			~uiWellImportSEGYVSP();

    void		use(const SeisTrc&);

protected:

    uiGenInput*		inpsampfld_;
    uiGenInput*		istimefld_;
    uiGenInput*		istvdfld_;
    uiGenInput*		unitfld_;
    uiGenInput*		lognmfld_;
    uiGenInput*		outsampfld_;
    uiCheckBox*		inpinftfld_;
    uiCheckBox*		outinftfld_;
    uiSEGYVSPBasicPars*	bparsfld_;
    uiIOObjSel*		wellfld_;

    IOPar		sgypars_;
    CtxtIOObj&		ctio_;
    SamplingData<float>	dispinpsamp_;
    BufferStringSet	existinglognms_;

    bool		inpIsTime() const;
    void		isTimeChg(CallBacker*);
    void		wllSel(CallBacker*);
    void		selLogNm(CallBacker*);
    void		outSampChk(CallBacker*);

    bool		acceptOK(CallBacker*);
    bool		fetchTrc(SeisTrc&);
    bool		createLog(const SeisTrc&,const StepInterval<float>&,
	    			  const char*);

    friend class	uiSEGYVSPBasicPars;

};

#endif
