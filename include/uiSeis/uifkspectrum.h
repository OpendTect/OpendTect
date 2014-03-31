#ifndef uifkspectrum_h
#define uifkspectrum_h

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          September 2007
RCS:           $Id$
______________________________________________________________________

*/

#include "uiseismod.h"
#include "uiflatviewmainwin.h"
#include "datapack.h"
#include "survinfo.h"
#include "odcomplex.h"

namespace Fourier { class CC; }
template <class T> class Array2D;
namespace FlatView { class AuxData; }
class uiGenInput;
class uiToolButton;

mExpClass(uiSeis) uiFKSpectrum : public uiFlatViewMainWin
{
public:
				uiFKSpectrum(uiParent*,bool setbp=false);
				~uiFKSpectrum();

    void			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const Array2D<float>&);

    float			getMinValue() const;
    float			getMaxValue() const;

protected:

    void			initFFT(int,int);
    bool			compute(const Array2D<float>&);
    bool			view(Array2D<float>&);
    FlatView::AuxData*		initAuxData();
    void			setVelCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);

    Fourier::CC*		fft_;
    Array2D<float_complex>*	input_;
    Array2D<float_complex>*	output_;
    Array2D<float>*		spectrum_;
    FlatView::AuxData*		lineitm_;
    FlatView::AuxData*		minvelitm_;
    FlatView::AuxData*		maxvelitm_;

    uiGenInput*			ffld_;
    uiGenInput*			kfld_;
    uiGenInput*			velfld_;

    uiGenInput*			minfld_;
    uiToolButton*		minsetbut_;
    uiGenInput*			maxfld_;
    uiToolButton*		maxsetbut_;
};

#endif

