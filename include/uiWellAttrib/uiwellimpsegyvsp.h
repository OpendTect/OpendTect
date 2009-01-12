#ifndef uiwellimpsegyvsp_h
#define uiwellimpsegyvsp_h
/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
 RCS:           $Id: uiwellimpsegyvsp.h,v 1.3 2009-01-12 12:42:45 cvsbert Exp $
 _______________________________________________________________________

      -*/


#include "uidialog.h"
#include "iopar.h"
#include "samplingdata.h"
class uiGenInput;
class uiIOObjSel;
class uiSEGYVSPBasicPars;
class SeisTrc;
class CtxtIOObj;


mClass uiWellImportSEGYVSP : public uiDialog
{
public:
    				uiWellImportSEGYVSP(uiParent*);
				~uiWellImportSEGYVSP();

    void			use(const SeisTrc&);

protected:

    uiGenInput*			inpsampfld_;
    uiGenInput*			istimefld_;
    uiGenInput*			istvdfld_;
    uiGenInput*			unitfld_;
    uiGenInput*			outsampfld_;
    uiGenInput*			lognmfld_;
    uiSEGYVSPBasicPars*		bparsfld_;
    uiIOObjSel*			wellfld_;
    SamplingData<float>		dispinpsamp_;

    IOPar			sgypars_;
    CtxtIOObj&			ctio_;

    bool			inpIsTime() const;
    void			isTimeChg(CallBacker*);
    void			initWin(CallBacker*);
    bool			acceptOK(CallBacker*);

    friend class		uiSEGYVSPBasicPars;

};

#endif
