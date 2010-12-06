#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.3 2010-12-06 16:16:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class SeisTrcBuf;
class uiSeisWaveletSel;
class uiFlatViewer;
class AIModel;
namespace Strat { class LayerModel; }


mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    void		setDispEach( int nr )		{ dispeach_ = nr; }
    void		modelChanged();

protected:

    const Strat::LayerModel& lm_;
    ObjectSet<AIModel>	aimdls_;
    int			dispeach_;

    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;

    void		emptyPacks();
};


#endif
