#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigroup.h"
#include "flatview.h"
#include "threadwork.h"
#include "uiworld2ui.h"

namespace FlatView { class uiAuxDataDisplay; }

class AxesDrawer;
class MouseEventHandler;
class uiBitMapDisplay;
class uiFlatViewControl;
class uiGraphicsItemGroup;
class uiGraphicsView;

/*!
\brief Fulfills the FlatView::Viewer specifications using 'ui' classes.
*/

mExpClass(uiFlatView) uiFlatViewer : public uiGroup
				   , public FlatView::Viewer
{
public:
    			uiFlatViewer(uiParent*);
			~uiFlatViewer();

    void		setInitialSize(const uiSize&);

    int			getAnnotChoices(BufferStringSet&) const;
    void		setAnnotChoice(int);

    MouseEventHandler&	getMouseEventHandler();
    uiGraphicsView&	rgbCanvas()			{ return *view_; }

    void		setView(const uiWorldRect&);
    void		setViewToBoundingBox();
    const uiWorldRect&	curView() const			{ return wr_; }
    			/*!<May be reversed if display is reversed. */
    StepInterval<double> posRange(bool forx1) const;
    uiWorldRect		boundingBox() const;

    const uiWorld2Ui&	getWorld2Ui() const		{ return w2ui_; }
    uiRect		getViewRect(bool withextraborders=true) const;
    			/*!<The rectangle onto which wr_ is projected */

    uiBorder		getAnnotBorder() const;
    void		setBoundingRect(const uiRect&);
    			/*!< Sets extra borders on the right and at the bottom
			 if boundingrect is smaller than getViewRect(false).
			 Extraborders set will be same as their differences in
			 width and height. */
    void		setExtraBorders(const uiSize& lt,const uiSize& rb);
    void		setExtraFactor( float f )	{ extfac_ = f; }
    			//!< when reporting boundingBox(), extends this
    			//!< amount of positions outward. Default 0.5.
    void		updateBitmapsOnResize( bool yn )
			{ updatebitmapsonresize_ = yn; }
			//!< If true, will resize bitmaps as per the size of
			//!< the window without maintaining the aspect ratio.
			//!< Else it is the responsibility of the programmer to
			//!< change view on resize.
    bool		updatesBitmapsOnResize() const
			{ return updatebitmapsonresize_; }

    void		handleChange(unsigned int);
    void		setSeisGeomidsToViewer(TypeSet<Pos::GeomID>&);

    FlatView::AuxData*		createAuxData(const char* nm) const;
    int				nrAuxData() const;
    FlatView::AuxData*		getAuxData(int idx);
    const FlatView::AuxData*	getAuxData(int idx) const;
    void			addAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(int idx);
    void			reGenerate(FlatView::AuxData&);

    AxesDrawer&			getAxesDrawer()		{ return axesdrawer_; }

    Notifier<uiFlatViewer> 	viewChanged; //!< setView called
    Notifier<uiFlatViewer> 	dataChanged; //!< new DataPack set
    Notifier<uiFlatViewer> 	dispParsChanged;
    					//!< Triggered with each bitmap update
    Notifier<uiFlatViewer>	annotChanged; //!< Annotation changed
    Notifier<uiFlatViewer>	dispPropChanged;
    					//!< Triggered with property dlg change

    uiFlatViewControl*		control() { return control_; }

			//restrain the data ranges between the selected ranges
    void		setUseSelDataRanges(bool yn) { useseldataranges_ = yn; }
    void		setSelDataRanges(Interval<double>,Interval<double>);
    const Interval<double>& getSelDataRange(bool forx) const
			{ return forx ? xseldatarange_ : yseldatarange_; }
    static int		bitMapZVal()			{ return 0; }
    static int		auxDataZVal()			{ return 100; }
    static int		annotZVal()			{ return 200; }

    const FlatPosData*	getFlatPosData(bool iswva);

protected:


    uiGraphicsView*		view_;
    AxesDrawer&			axesdrawer_; //!< Must be declared after canvas_
    uiWorldRect			wr_;
				/*!<May be reversed if display is reversed. */
    uiGraphicsItemGroup*	worldgroup_;
    uiWorld2Ui			w2ui_;

    uiBitMapDisplay*		bitmapdisp_;

    Threads::Atomic<bool>	updateannot_;
    Threads::Atomic<bool>	updatebitmap_;
    Threads::Atomic<bool>	updateauxdata_;

    void			updateCB(CallBacker*);
    void			updateAnnotCB(CallBacker*);
    void			updateBitmapCB(CallBacker*);
    void			updateAuxDataCB(CallBacker*);

    uiWorldRect			getBoundingBox(bool wva) const;
    void			reSizeCB(CallBacker*);

    bool			updatebitmapsonresize_;
    float			extfac_;

    void			updateTransforms();

    uiFlatViewControl*		control_;
    friend class		uiFlatViewControl;

    Interval<double>		xseldatarange_;
    Interval<double>		yseldatarange_;
    bool			useseldataranges_;

    ObjectSet<FlatView::uiAuxDataDisplay>	auxdata_;

public:
    uiBitMapDisplay*	bitmapDisp()			{ return bitmapdisp_; }
protected:
    void			rangeUpdatedCB(CallBacker*);

};

