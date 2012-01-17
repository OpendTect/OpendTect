#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.13 2012-01-17 11:12:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiStratLayModEditTools;
namespace Strat { class LayerModel; }


/*!\brief Strat: Layer Model Displayer

 The world rect boundaries are [1,nrmodels+1] vs zrg_.

  */

mClass uiStratLayerModelDisp : public uiGroup
{
public:

				uiStratLayerModelDisp(uiStratLayModEditTools&,
						    const Strat::LayerModel&);
				~uiStratLayerModelDisp();

    virtual void		modelChanged()			= 0;
    virtual void		setZoomBox(const uiWorldRect&)	= 0;

    const TypeSet<float>&	levelDepths() const	{ return lvldpths_; }
    void			selectSequence(int seqidx);

    Notifier<uiStratLayerModelDisp> sequenceSelected;

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
