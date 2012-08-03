#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.17 2012-08-03 13:01:10 cvskris Exp $
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uigroup.h"
class uiStratLayModEditTools;
namespace Strat { class LayerModel; }


/*!\brief Strat: Layer Model Displayer

 The world rect boundaries are [1,nrmodels+1] vs zrg_.

  */

mClass(uiStrat) uiStratLayerModelDisp : public uiGroup
{
public:

				uiStratLayerModelDisp(uiStratLayModEditTools&,
						    const Strat::LayerModel&);
				~uiStratLayerModelDisp();

    virtual void		modelChanged()			= 0;
    virtual void		setZoomBox(const uiWorldRect&)	= 0;

    const TypeSet<float>&	levelDepths() const	{ return lvldpths_; }
    void			selectSequence(int seqidx);

    virtual uiBaseObject*	getViewer() { return 0; }

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> rangeChanged;

protected:

    const Strat::LayerModel& lm_;
    uiStratLayModEditTools& tools_;
    uiWorldRect		zoomwr_;
    int			selseqidx_;
    TypeSet<float>	lvldpths_;
    Interval<float>	zrg_;

    bool		haveAnyZoom() const;
    virtual void	drawSelectedSequence()		= 0;

};


#endif

