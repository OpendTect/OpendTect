#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.7 2010-12-21 13:19:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class SeisTrcBuf;
class uiSeisWaveletSel;
class uiFlatViewer;
class AIModel;
class Wavelet;
namespace Strat { class LayerModel; }


mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    void		setDispEach(int);
    void		setDispMrkrs(const TypeSet<float>&,Color);
    void		modelChanged();
    const uiWorldRect&	curView(bool indepth) const;

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	zoomChanged;

protected:

    const Strat::LayerModel& lm_;
    ObjectSet<AIModel>	aimdls_;
    Wavelet*		wvlt_;
    int			dispeach_;
    int			longestaimdl_;

    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;

    void		wvltChg(CallBacker*);
    void		zoomChg(CallBacker*);
    int			getVelIdx(bool&) const;
    int			getDenIdx(bool&) const;

};


#endif
