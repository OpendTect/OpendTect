#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "position.h"
#include "uiobjectitemview.h"
#include "uigroup.h"

class uiFlatViewer;

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

    virtual void		setPosition(const BinID&,
					    const Interval<double>* zrg=0);
    void			setVDGather(int);
    void			setWVAGather(int);

    void			displayAnnotation(bool yn);
    bool			displaysAnnotation() const;

    void			setFixedOffsetRange(bool yn,
						    const Interval<float>&);
    bool			getFixedOffsetRange() const;
    const Interval<float>&	getOffsetRange() const;
    const Interval<double>*	getZRange() const	{ return zrg_; }
    const Interval<float>&	getZDataRange() const	{ return zdatarange_; }

    uiFlatViewer*		getUiFlatViewer() 	{ return viewer_; }
    BinID			getBinID() const;

    void			setInitialSize(const uiSize&);
    void			setWidth(int);
    void			updateViewRange();

protected:

    uiFlatViewer*		viewer_;
    Viewer2DGatherPainter* 	gatherpainter_;

    bool			fixedoffset_;
    Interval<float>		offsetrange_;
    Interval<float>		zdatarange_;
    Interval<double>*		zrg_;
    BinID			bid_;
    bool			displayannotation_;

    void			updateViewRange(const uiWorldRect&);
};



mExpClass(uiPreStackProcessing) uiViewer2D : public uiObjectItemView
{
public:
				uiViewer2D(uiParent*);
				~uiViewer2D();

    uiGatherDisplay*		addGatherDisplay(int vdid, int wvaid=-1);
    void			addGatherDisplay(uiGatherDisplay*);
    void 			removeGatherDisplay(const uiGatherDisplay*);
    uiGatherDisplay& 		getGatherDisplay(int idx);
    uiGatherDisplay* 		getGatherDisplay(const BinID&);
    void			removeAllGatherDisplays();
    void			enableScrollBars(bool);
    void			enableReSizeDraw(bool);
    void			doReSize(const uiSize&);
    void			doReSize()			{ reSized(0); }

    protected:

    bool			resizedraw_;
    void			reSized(CallBacker*);
};

} // namespace PreStackView
