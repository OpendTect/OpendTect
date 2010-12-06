#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.2 2010-12-06 12:18:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class SeisTrcBuf;
class uiSeisWaveletSel;
class uiFlatViewer;
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
    SeisTrcBuf&		tbuf_;
    int			dispeach_;

    uiSeisWaveletSel*	wvltfld_;
    uiFlatViewer*	vwr_;

};


#endif
