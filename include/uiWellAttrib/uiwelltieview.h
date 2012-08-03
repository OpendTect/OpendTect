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

#include "uiwellattribmod.h"
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

mClass(uiWellAttrib) uiTieView : public CallBacker
{
public:
			    	uiTieView(uiParent*,uiFlatViewer*,const Data&);
			    	~uiTieView();

    void        		fullRedraw();
    void        		drawUserPicks();
    void 			redrawViewer();
    void 			redrawViewerAuxDatas();
    void 			redrawLogsAuxDatas();

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

    ObjectSet<FlatView::AuxData> userpickauxdatas_;
    ObjectSet<FlatView::AuxData> wellmarkerauxdatas_;
    ObjectSet<FlatView::AuxData> horauxdatas_;
    ObjectSet<uiTextItem> 	hortxtnms_;
    ObjectSet<uiTextItem> 	mrktxtnms_;
    uiPolyLineItem*		checkshotitm_;

    void        		drawLog(const char*,bool,int,bool);
    void        		drawTraces();
    void			drawUserPicks(const TypeSet<Marker>&,bool);
    void        		drawMarker(FlatView::AuxData*,
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



mClass(uiWellAttrib) uiCrossCorrView : uiGroup
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


