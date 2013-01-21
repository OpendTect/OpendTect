#ifndef uiflatviewer_h
#define uiflatviewer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigroup.h"
#include "flatview.h"
#include "threadwork.h"

namespace FlatView 
{
    class AxesDrawer;
    class uiAuxDataDisplay;
    class uiBitMapDisplay;
}


class uiGraphicsView;
class BufferStringSet;
class uiFlatViewControl;
class uiWorld2Ui;
class uiGraphicsItemGroup;


/*!\brief Fulfills the FlatView::Viewer specifications using 'ui' classes. */

mExpClass(uiFlatView) uiFlatViewer : public uiGroup
		    , public FlatView::Viewer
{
public:
    			uiFlatViewer(uiParent*);
			~uiFlatViewer();

    void		setInitialSize(uiSize);
    void		setRubberBandingOn(bool);

    int			getAnnotChoices(BufferStringSet&) const;
    void		setAnnotChoice(int);

    uiGraphicsView&	rgbCanvas()			{ return *view_; }

    void		setView(const uiWorldRect&);
    void		setViewToBoundingBox();
    const uiWorldRect&	curView() const			{ return wr_; }
    uiWorldRect		boundingBox() const;

    void		getWorld2Ui(uiWorld2Ui&) const;
    uiRect		getViewRect() const;
    			/*!<The rectangle onto which wr_ is projected */
    void		setExtraBorders(const uiSize& lt,const uiSize& rb);
    void		setExtraBorders( uiRect r )	{ extraborders_ = r; }
    void		setDim0ExtFac( float f )	{ dim0extfac_ = f; }
    			//!< when reporting boundingBox(), extends this
    			//!< amount of positions outward. Default 0.5.

    void		handleChange(DataChangeType,bool dofill = true);

    FlatView::AuxData*		createAuxData(const char* nm) const;
    int				nrAuxData() const;
    FlatView::AuxData*		getAuxData(int idx);
    const FlatView::AuxData*	getAuxData(int idx) const;
    void			addAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(int idx);
    void			reGenerate(FlatView::AuxData&);

    Notifier<uiFlatViewer> 	viewChanged; //!< setView called
    Notifier<uiFlatViewer> 	dataChanged; //!< new DataPack set

    uiFlatViewControl*		control() { return control_; }


			//restrain the data ranges between the selected ranges
    void		setUseSelDataRanges(bool yn) { useseldataranges_ = yn; }
    void		setSelDataRanges(Interval<double>,Interval<double>);
    const Interval<double>& getSelDataRange(bool forx) const
			{ return forx ? xseldatarange_ : yseldatarange_; } 

    void		setNoViewDone() {}

protected:

    uiGraphicsView*		view_;
    FlatView::AxesDrawer& 	axesdrawer_; //!< Must be declared after canvas_
    uiWorldRect			wr_;
    uiGraphicsItemGroup*	worldgroup_;

    TypeSet<DataChangeType>	reportedchanges_;
    float			dim0extfac_;
    uiRect			extraborders_;

    FlatView::uiBitMapDisplay*	bitmapdisp_;

    Threads::Work		annotwork_;
    Threads::Work		bitmapwork_;
    Threads::Work		auxdatawork_;

    void			updateCB(CallBacker*);
    void			updateAnnotCB(CallBacker*);
    void			updateBitmapCB(CallBacker*);
    void			updateAuxDataCB(CallBacker*);

    uiWorldRect			getBoundingBox(bool) const;
    void			reSizeCB(CallBacker*);

    int				updatequeueid_;

    void			updateTransforms();

    uiFlatViewControl*		control_;
    friend class		uiFlatViewControl;

    Interval<double>		xseldatarange_;
    Interval<double>		yseldatarange_;
    bool			useseldataranges_;

    ObjectSet<FlatView::uiAuxDataDisplay>	auxdata_;

public:
				//od4.4 legacy, remove when not used anymore
    				uiFlatViewer(uiParent*,bool yn); 
};

#endif

