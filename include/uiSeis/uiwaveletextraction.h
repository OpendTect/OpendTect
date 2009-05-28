/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwaveletextraction.h,v 1.3 2009-05-28 09:42:50 cvsnageswara Exp $

#include "uidialog.h"
#include <complex>

class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSelZRange;
class uiPosProvGroup;
class uiSeis3DSubSel;
typedef std::complex<float> float_complex;
template <class T> class Array1DImpl;
class CtxtIOObj;
class IOPar;
class SeisTrcBuf;
class MultiID;

class uiWaveletExtraction : public uiDialog
{
public:
				uiWaveletExtraction(uiParent*);
    MultiID			storeKey() const;
protected:

    void			choiceSel(CallBacker*);
    void			inputSel(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			doProcess(const IOPar&);
    bool                        readInputData(const IOPar&);
    bool			doFFT(const SeisTrcBuf&,float*);
    bool			doIFFT(const float*,float*);
    void			normalisation(Array1DImpl<float>&);
    void			storeWavelet(const float*);

    CtxtIOObj&			seisctio_;
    CtxtIOObj&			wvltctio_;
    SeisTrcBuf*			seistrcbuf_;
    uiGenInput*			zextraction_;
    uiGenInput*			wtlengthfld_;
    uiIOObjSel*			outputwvltfld_;
    uiSeisSel*			seisselfld_;
    uiSelZRange*		zrangefld_;
    uiSeis3DSubSel*		subselfld_;
    uiPosProvGroup* 		surfacesel_;
    int				zextractval_;
    int				wvltsize_;
};
