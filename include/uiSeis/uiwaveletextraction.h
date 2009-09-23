/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
 RCS:		$Id: uiwaveletextraction.h,v 1.10 2009-09-23 05:56:05 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiPosProvGroup;
class uiSeisSel;
class uiSeis3DSubSel;
class uiSelZRange;
class CtxtIOObj;
class IOPar;
class MultiID;
namespace Seis { class SelData; class TableSelData; }


mClass uiWaveletExtraction : public uiDialog
{
public:
				uiWaveletExtraction(uiParent*);
				~uiWaveletExtraction();
    MultiID			storeKey() const;

    Notifier<uiWaveletExtraction>	extractionDone;

protected:

    void			choiceSel(CallBacker*);
    void			inputSel(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			doProcess(const IOPar&,const IOPar&);
    bool                        readInputData(const IOPar&,const IOPar&);
    bool			fillHorizonSelData(const IOPar&,const IOPar&,
						   Seis::TableSelData&);
    bool			storeWavelet(const float*);

    CtxtIOObj&			seisctio_;
    CtxtIOObj&			wvltctio_;
    uiGenInput*			zextraction_;
    uiGenInput*			wtlengthfld_;
    uiGenInput*			wvltphasefld_;
    uiGenInput*			taperfld_;
    uiIOObjSel*			outputwvltfld_;
    uiSeisSel*			seisselfld_;
    uiSelZRange*		zrangefld_;
    uiSeis3DSubSel*		subselfld_;
    uiPosProvGroup* 		surfacesel_;
    Seis::SelData*		sd_;

    int				wvltsize_;
    bool			betweenhors_;

};
