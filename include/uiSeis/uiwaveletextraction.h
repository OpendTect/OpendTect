/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
 RCS:		$Id: uiwaveletextraction.h,v 1.8 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include <complex>

class uiGenInput;
class uiIOObjSel;
class uiPosProvGroup;
class uiSeisSel;
class uiSeis3DSubSel;
class uiSelZRange;
class BinIDValueSet;
class CtxtIOObj;
class IOPar;
class MultiID;
namespace Seis { class SelData; class TableSelData; }
class SeisTrcBuf;
typedef std::complex<float> float_complex;
template <class T> class Array1DImpl;

mClass uiWaveletExtraction : public uiDialog
{
public:
				uiWaveletExtraction(uiParent*);
    MultiID			storeKey() const;
protected:

    void			choiceSel(CallBacker*);
    void			inputSel(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			doProcess(const IOPar&,const IOPar&);
    bool                        readInputData(const IOPar&,const IOPar&
	    				      ,Seis::SelData*&);
    bool			doFFT(const SeisTrcBuf&,const Seis::SelData*
				      ,float*);
    bool			doIFFT(const float*,float*);
    void			normalisation(Array1DImpl<float>&);
    void			storeWavelet(const float*);
    bool			fillHorizonSelData(const IOPar&
	    					   ,const IOPar&
						   ,Seis::TableSelData&);
    bool			calcWvltPhase(const float*,float*);

    CtxtIOObj&			seisctio_;
    CtxtIOObj&			wvltctio_;
    SeisTrcBuf*			seistrcbuf_;
    uiGenInput*			zextraction_;
    uiGenInput*			wtlengthfld_;
    uiGenInput*			wvltphasefld_;
    uiIOObjSel*			outputwvltfld_;
    uiSeisSel*			seisselfld_;
    uiSelZRange*		zrangefld_;
    uiSeis3DSubSel*		subselfld_;
    uiPosProvGroup* 		surfacesel_;
    int				zextractval_;
    int				wvltsize_;
    bool			isdouble_;
};
