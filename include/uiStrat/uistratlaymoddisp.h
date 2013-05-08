#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uigroup.h"

class PropertyRef;
class uiStratLayModEditTools;
namespace Strat { class LayerModel; class LayerModelProvider; class Layer; }

/*!
\brief Strat Layer Model Displayer

  The world rect boundaries are [1,nrmodels+1] vs zrg_.
*/

mExpClass(uiStrat) uiStratLayerModelDisp : public uiGroup
{
public:

			uiStratLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModelProvider&);
			~uiStratLayerModelDisp();

    virtual void	modelChanged()			= 0;
    virtual uiWorldRect	zoomBox() const			= 0;
    virtual void	setZoomBox(const uiWorldRect&)	= 0;
    virtual float	getDisplayZSkip() const		= 0;

    const Strat::LayerModel& layerModel() const;
    const TypeSet<float>& levelDepths() const		{ return lvldpths_; }
    int			selectedSequence() const	{ return selseqidx_; }
    void		selectSequence(int seqidx);

    virtual uiBaseObject* getViewer() { return 0; }
    bool		isFlattened() const		{ return flattened_; }
    void		setFlattened(bool yn);
    bool		isFluidReplOn() const		{ return fluidreplon_; }
    void		setFluidReplOn(bool yn)		{ fluidreplon_= yn; }

    float		getLayerPropValue(const Strat::Layer&,
	    				  const PropertyRef*,int) const;

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> rangeChanged;
    Notifier<uiStratLayerModelDisp> modelEdited;
    CNotifier<uiStratLayerModelDisp,IOPar> infoChanged;

protected:

    const Strat::LayerModelProvider& lmp_;
    uiStratLayModEditTools& tools_;
    int			selseqidx_;
    Interval<float>	zrg_;
    bool		flattened_;
    bool		fluidreplon_;
    TypeSet<float>	lvldpths_;

    bool		haveAnyZoom() const;
    virtual void	drawSelectedSequence()		= 0;

};


#endif

