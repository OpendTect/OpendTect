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
#include "welltieunitfactors.h"
#include "welltiedata.h"

class SeisTrc;
class SeisTrcBuf;
class SeisTrcBufDataPack;

class uiFlatViewer;
class uiFunctionDisplay;
class uiPolyLineItem;
class uiWellLogDisplay;
class uiLabel;
class uiTextItem;
class uiWellDisplayControl;

namespace Well
{
    class Marker;
    class Data;
};

namespace EM
{
    class Horizon2D;
    class Horizon3D;
}

namespace WellTie
{
    class Setup; 
    class DataHolder; 
    class PickSet; 

mClass uiTieView : public CallBacker
{
public:
			    	uiTieView(uiParent*,uiFlatViewer*,
					  WellTie::DataHolder&,
					  ObjectSet<uiWellLogDisplay>*);
				~uiTieView();

    void        		fullRedraw();
    void 			redrawViewer(CallBacker*);
    void 			redrawViewerMarkers(CallBacker*);
    void        		drawUserPicks();
    bool        		isEmpty(); 

    Notifier<uiTieView> 	infoMsgChanged;

protected:

    ObjectSet<uiWellLogDisplay>& logsdisp_;
    DataHolder&  		dataholder_;
    const Setup& 		wtsetup_;
    const Params::DataParams* 	params_;
    uiWellDisplayControl*       wellcontrol_;

    PickSet*			seispickset_;
    PickSet*			synthpickset_;

    uiFlatViewer*		vwr_;
    SeisTrcBuf*			trcbuf_;
    SeisTrcBufDataPack*		seistrcdp_;
    ObjectSet<SeisTrc>		trcs_;
    const StepInterval<float>	zrange_;
    float			maxtraceval_;
    float			mintraceval_;

    ObjectSet<FlatView::Annotation::AuxData> userpickauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> wellmarkerauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> horauxdatas_;
    ObjectSet<uiTextItem> 	hortxtnms_;
    ObjectSet<uiTextItem> 	mrktxtnms_;
    uiPolyLineItem*		checkshotitm_;

    void        		drawAILog();
    void        		drawVelLog();
    void        		drawDenLog();
    void        		drawRefLog();
    void        		drawTraces();
    void			drawUserPicks(const WellTie::PickSet*);
    void        		drawMarker(FlatView::Annotation::AuxData*,
					    bool,float,Color,bool,bool);
    void        		drawViewerWellMarkers();
    void        		drawLogDispWellMarkers();
    void        		drawCShot();
    void        		initFlatViewer();
    void        		initLogViewers();
    void        		initWellControl();
    void			loadHorizons();
    void			drawHorizons();
    void 			setLogsRanges(float,float);
    bool 			setLogsParams();
    void 			setUpTrcBuf(SeisTrcBuf*,const char*,int);
    void			setUpUdfTrc(SeisTrc&,const char*,int);
    void			setUpValTrc(SeisTrc&,const char*,int);
    void        		setDataPack(SeisTrcBuf*,const char*,int);
    void 			setInfoMsg(CallBacker*);
    void			zoomChg(CallBacker*);
};



mClass uiCorrView : uiGroup
{
public:

				uiCorrView(uiParent*,WellTie::DataHolder&);
				~uiCorrView();

    void                	setCrossCorrelation();

protected:

    uiLabel* 			corrlbl_;
    WellTie::DataHolder& 	dataholder_;
    ObjectSet<uiFunctionDisplay>  corrdisps_;
};

}; //namespace WellTie
#endif

