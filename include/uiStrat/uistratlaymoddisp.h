#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
________________________________________________________________________

-*/

#include "flatview.h"
#include "uistratmod.h"
#include "uigroup.h"
#include "uistring.h"

class PropertyRef;
class uiGraphicsScene;
class uiStratLayModEditTools;
class uiTextItem;
class uiFlatViewer;
namespace Strat { class LayerModel; class LayerModelProvider; class Layer; }

/*!
\brief Strat Layer Model Displayer

  The world rect boundaries are [1,nrmodels+1] vs zrg_.
*/

mStruct(uiStrat) LMPropSpecificDispPars
{
			LMPropSpecificDispPars( const char* nm=0 )
			    : propnm_(nm)	{}
    bool		operator==( const LMPropSpecificDispPars& oth ) const
			{ return propnm_ == oth.propnm_; }

    ColTab::MapperSetup	mapper_;
    BufferString	ctab_;
    float		overlap_;
    BufferString	propnm_;
};


mExpClass(uiStrat) uiStratLayerModelDisp : public uiGroup
{ mODTextTranslationClass(uiStratLayerModelDisp)
public:
			~uiStratLayerModelDisp();

    virtual void	modelChanged()			= 0;
    virtual void	reSetView()			= 0;
    void		modelUpdate()	{ modelChanged(); reSetView(); }
    virtual uiWorldRect	zoomBox() const			= 0;
    virtual void	setZoomBox(const uiWorldRect&)	= 0;
    virtual float	getDisplayZSkip() const		= 0;
    uiGroup*		getDisplayClone(uiParent*) const;
    virtual Interval<float> relDepthZoneOfInterest() const
			{ return Interval<float>::udf(); }
    virtual void	reSetRelDepthZoneOfInterest()	{}
    virtual bool	canSetDisplayProperties() const	{ return false; }
    virtual void	savePars()			{}
    virtual void	retrievePars()			{}

    const Strat::LayerModel& layerModel() const;
    const TypeSet<float>& levelDepths() const		{ return lvldpths_; }
    int			selectedSequence() const	{ return selseqidx_; }
    void		selectSequence(int seqidx);

    uiFlatViewer*	getViewer() { return &vwr_; }
    bool		isFlattened() const		{ return flattened_; }
    void		setFlattened(bool yn,bool trigger=true);
    mDeprecatedDef bool isFluidReplOn() const		{ return fluidreplon_; }
    mDeprecatedDef void setFluidReplOn(bool yn)		{ fluidreplon_= yn; }
    mDeprecatedDef bool isBrineFilled() const		{return isbrinefilled_;}
    mDeprecatedDef void setBrineFilled(bool yn)		{ isbrinefilled_= yn; }
    void		displayFRText(bool yn,bool isbrine);

    float		getLayerPropValue(const Strat::Layer&,
					  const PropertyRef*,int) const;
    bool		setPropDispPars(const LMPropSpecificDispPars&);
    bool		getCurPropDispPars(LMPropSpecificDispPars&) const;
    void		clearDispPars()		{ lmdisppars_.erase(); }

    void		setGenDescKey( const MultiID& key ) { gendesckey_=key; }

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> rangeChanged;
    Notifier<uiStratLayerModelDisp> modelEdited;
    CNotifier<uiStratLayerModelDisp,IOPar> infoChanged;
    Notifier<uiStratLayerModelDisp> dispPropChanged;

protected:
				uiStratLayerModelDisp(uiStratLayModEditTools&,
					const Strat::LayerModelProvider&);

    uiFlatViewer&		vwr_;
    const Strat::LayerModelProvider& lmp_;
    uiStratLayModEditTools&	tools_;
    uiTextItem*			frtxtitm_;
    int				selseqidx_;
    Interval<float>		zrg_;
    bool			flattened_;
    /*mDeprecated*/ bool	fluidreplon_;
    /*mDeprecated*/ bool	isbrinefilled_;
    TypeSet<float>		lvldpths_;
    TypeSet<LMPropSpecificDispPars> lmdisppars_;
    IOPar			dumppars_;
    MultiID			gendesckey_;

    bool			haveAnyZoom() const;
    uiGraphicsScene&		scene() const;
    mDeprecatedDef void		displayFRText();
    virtual void		drawSelectedSequence()		= 0;

    int				getClickedModelNr() const;
    void			mouseMoved(CallBacker*);
    void			updateTextPosCB(CallBacker*);
    void			doubleClicked(CallBacker*);
    void			usrClicked(CallBacker*);
    virtual void		selPropChgCB(CallBacker*)	= 0;
    virtual void		dispLithChgCB(CallBacker*)	= 0;
    virtual void		selContentChgCB(CallBacker*)	= 0;
    virtual void		selLevelChgCB(CallBacker*)	= 0;
    virtual void		dispEachChgCB(CallBacker*)	= 0;
    virtual void		dispZoomedChgCB(CallBacker*)	= 0;
    bool			doLayerModelIO(bool);
    virtual void		doLevelChg()			= 0;
    virtual void		handleClick(bool dble)		= 0;
				//!< returns whether layermodel has changed
};

