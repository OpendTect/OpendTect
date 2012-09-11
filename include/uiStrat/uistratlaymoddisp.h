#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.19 2012-09-11 11:02:11 cvsbert Exp $
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

    virtual void	modelChanged()			= 0;
    virtual void	setZoomBox(const uiWorldRect&)	= 0;

    const TypeSet<float>& levelDepths() const		{ return lvldpths_; }
    int			selectedSequence() const	{ return selseqidx_; }
    void		selectSequence(int seqidx);

    virtual uiBaseObject* getViewer() { return 0; }
    bool		isFlattened() const		{ return flattened_; }
    void		setFlattened(bool yn);

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> rangeChanged;
    Notifier<uiStratLayerModelDisp> modelEdited;

protected:

    const Strat::LayerModel& lm_;
    uiStratLayModEditTools& tools_;
    uiWorldRect		zoomwr_;
    int			selseqidx_;
    bool		flattened_;
    Interval<float>	zrg_;
    TypeSet<float>	lvldpths_;

    bool		haveAnyZoom() const;
    virtual void	drawSelectedSequence()		= 0;

};


#endif

