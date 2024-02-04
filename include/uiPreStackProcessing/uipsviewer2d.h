#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "datapack.h"
#include "position.h"
#include "uiobjectitemview.h"
#include "uigroup.h"

class uiFlatViewer;
namespace PreStack { class Gather; }

namespace PreStackView
{
class Viewer2DGatherPainter;
class uiViewer2DAxisPainter;

/*!
\brief Displays multiple PreStack Gathers side by side with dynamic redraw
possibility.
*/

mExpClass(uiPreStackProcessing) uiGatherDisplay : public uiGroup
{
public:
				uiGatherDisplay(uiParent*);
				~uiGatherDisplay();

    virtual void		setZRange(const Interval<double>* zrg=nullptr);
    mDeprecated("No longer used")
    void			setVDGather(DataPackID);
    mDeprecated("No longer used")
    void			setWVAGather(DataPackID);
    void			setVDGather(const PreStack::Gather*);
    void			setWVAGather(const PreStack::Gather*);

    void			displayAnnotation(bool yn);
    bool			displaysAnnotation() const;

    void			setFixedOffsetRange(bool yn,
				    const Interval<float>&);
    bool			getFixedOffsetRange() const;
    TrcKey			getTrcKey() const;
    const Interval<float>&	getOffsetRange() const;
    const Interval<double>*	getZRange() const	{ return zrg_; }
    const Interval<float>&	getZDataRange() const	{ return zdatarange_; }

    uiFlatViewer*		getUiFlatViewer()	{ return viewer_; }

    void			setInitialSize(const uiSize&);
    void			setWidth(int);
    void			updateViewRange();

protected:

    uiFlatViewer*		viewer_;
    Viewer2DGatherPainter*	gatherpainter_;

    bool			fixedoffset_	= false;
    Interval<float>		offsetrange_	= Interval<float>::udf();
    Interval<float>		zdatarange_;
    Interval<double>*		zrg_		= nullptr;
    bool			displayannotation_ = true;

    void			updateViewRange(const uiWorldRect&);
};



mExpClass(uiPreStackProcessing) uiViewer2D : public uiObjectItemView
{
public:
				uiViewer2D(uiParent*);
				~uiViewer2D();

    mDeprecated("No longer used")
    uiGatherDisplay*		addGatherDisplay(DataPackID vdid,
				 DataPackID wvaid=DataPackID::udf());
    uiGatherDisplay*		addGatherDisplay(PreStack::Gather* vd,
						 PreStack::Gather* wva=nullptr);
    void			addGatherDisplay(uiGatherDisplay*);
    void			removeGatherDisplay(const uiGatherDisplay*);
    uiGatherDisplay&		getGatherDisplay(int idx);
    uiGatherDisplay*		getGatherDisplay(const TrcKey&);
    void			removeAllGatherDisplays();
    void			enableScrollBars(bool);
    void			enableReSizeDraw(bool);
    void			doReSize(const uiSize&);
    void			doReSize()			{ reSized(0); }

    protected:

    bool			resizedraw_;
    void			reSized(CallBacker*);
};

} // namespace PreStack
