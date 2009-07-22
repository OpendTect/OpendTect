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
#include "flatview.h"
#include "welltieunitfactors.h"
#include "welltiedata.h"

template <class T> class Array1DImpl;
class WellTieSetup; 
class WellTiePickSet; 
class SeisTrc;
class SeisTrcBuf;
class SeisTrcBufDataPack;

class uiFlatViewer;
class uiFunctionDisplay;
class uiPolyLineItem;
class uiWellLogDisplay;
class uiLabel;
namespace Well
{
    class Marker;
    class Data;
};
mClass uiWellTieView : public CallBacker
{
public:
			    	uiWellTieView(uiParent*,uiFlatViewer*,
					      WellTieDataHolder*,
					      ObjectSet<uiWellLogDisplay>*);
				~uiWellTieView();

    void        		fullRedraw();
    void        		drawUserPicks();
    bool        		isEmpty() { return data_.isEmpty(); }


protected:

    uiFlatViewer*		vwr_;

    ObjectSet<uiWellLogDisplay>& logsdisp_;
    WellTieDataSetMGR&		datamgr_; 		
    WellTieDataSet&  		data_;
    WellTieDataHolder*  	dataholder_;
    const Well::Data& 		wd_;		
    const WellTieSetup& 	wtsetup_;
    const WellTieParams::DataParams* params_;
    WellTiePickSet*		seispickset_;
    WellTiePickSet*		synthpickset_;

    SeisTrcBuf*			trcbuf_;
    SeisTrcBufDataPack*		seistrcdp_;
    ObjectSet<SeisTrc>		trcs_;
    float			maxtraceval_;
    float			mintraceval_;

    ObjectSet<FlatView::Annotation::AuxData> userpickauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> wellmarkerauxdatas_;
    uiPolyLineItem*		checkshotitm_;

    void        		drawAILog();
    void        		drawVelLog();
    void        		drawDenLog();
    void        		drawRefLog();
    void        		drawTraces();
    void			drawUserPicks(const WellTiePickSet*);
    void        		drawMarker(FlatView::Annotation::AuxData*,
					    int,float,float,Color,bool);
    void        		drawWellMarkers();
    void        		drawCShot();
    void        		initFlatViewer();
    void        		initLogViewers();
    void 			removePack();
    void 			setLogsRanges(float,float);
    void 			setLogsParams();
    void 			setUpTrcBuf(SeisTrcBuf*,const char*,int);
    void			setUpUdfTrc(SeisTrc&,const char*,int);
    void			setUpValTrc(SeisTrc&,const char*,int);
    void        		setDataPack(SeisTrcBuf*,const char*,int);
    void			zoomChg(CallBacker*);
};



mClass uiWellTieCorrView : uiGroup
{
public:

				uiWellTieCorrView(uiParent*,WellTieDataHolder*);
				~uiWellTieCorrView();

    void                	setCrossCorrelation();

protected:

    uiLabel* 			corrlbl_;
    WellTieDataSet& 		corrdata_;
    const WellTieData& 		welltiedata_;
    const WellTieParams::DataParams& params_;
    ObjectSet<uiFunctionDisplay>  corrdisps_;
};

#endif

