#ifndef uiwelltieview_h
#define uiwelltieview_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: uiwelltieview.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uiflatviewer.h"
#include "welltiedata.h"

class SeisTrc;
class SeisTrcBuf;
class SeisTrcBufDataPack;
class uiFunctionDisplay;
class uiPolyLineItem;
class uiWellLogDisplay;
class uiLabel;
class uiTextItem;
class uiWellDisplayControl;

namespace Well
{
    class Data;
    class Marker;
};

namespace WellTie
{
    class Setup; 

mClass uiTieView : public CallBacker
{
public:
			    	uiTieView(uiParent*,uiFlatViewer*,const Data&);
			    	~uiTieView();

    void        		fullRedraw();
    void        		drawUserPicks();
    void 			redrawViewer();
    void 			redrawViewerAnnots();

    void			enableCtrlNotifiers(bool);

    ObjectSet<uiWellLogDisplay>& logDisps() { return logsdisp_; }

    Notifier<uiTieView> 	infoMsgChanged;

protected:

    uiFlatViewer*		vwr_;
    uiParent*			parent_;
    ObjectSet<uiWellLogDisplay> logsdisp_;
    uiWellDisplayControl*       wellcontrol_;

    const DispParams&		params_;
    const Data&			data_;
    const StepInterval<float>&	zrange_;
    const TypeSet<Marker>&	seispickset_;
    const TypeSet<Marker>&	synthpickset_;
    SeisTrcBuf&			trcbuf_;
    SeisTrcBufDataPack*		seisdp_;

    ObjectSet<FlatView::Annotation::AuxData> userpickauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> wellmarkerauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> horauxdatas_;
    ObjectSet<uiTextItem> 	hortxtnms_;
    ObjectSet<uiTextItem> 	mrktxtnms_;
    uiPolyLineItem*		checkshotitm_;

    void        		drawLog(const char*,bool,int,bool);
    void        		drawTraces();
    void			drawUserPicks(const TypeSet<Marker>&,bool);
    void        		drawMarker(FlatView::Annotation::AuxData*,
					    bool,float);
    void        		drawViewerWellMarkers();
    void        		drawLogDispWellMarkers();
    void        		initFlatViewer();
    void        		initLogViewers();
    void        		initWellControl();
    void			loadHorizons();
    void			drawHorizons();
    void 			setLogsRanges(Interval<float>);
    void 			setLogsParams();
    void			setUdfTrc(SeisTrc&) const;
    void        		setDataPack();
    void 			setInfoMsg(CallBacker*);
    void			zoomChg(CallBacker*);
};



mClass uiCrossCorrView : uiGroup
{
public:

				uiCrossCorrView(uiParent*,const Data&);

    void                	set(const Data::CorrelData&);
    void                	draw();

protected:

    uiLabel* 			lbl_;
    uiFunctionDisplay* 		disp_;
    TypeSet<float>		vals_;
    float			lag_;
    float			coeff_;
    const Data& 		data_;
};

}; //namespace WellTie
#endif

