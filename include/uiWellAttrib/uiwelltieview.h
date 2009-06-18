#ifndef uiwelltieview_h
#define uiwelltieview_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: uiwelltieview.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uigroup.h"
#include "flatview.h"
#include "welltieunitfactors.h"

template <class T> class Array1DImpl;
class WellTieSetup; 
class WellTieDataHolder; 
class WellTieDataSet; 
class WellTieData; 
class WellTieDataSetMGR; 
class WellTiePickSet; 
class SeisTrcBuf;
class SeisTrc;
class UserPick;
class UserPickSet;
namespace Attrib { class DescSet; }

class uiFlatViewer;
class uiFunctionDisplay;
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

    void        		drawAILog();
    void        		drawVelLog();
    void        		drawDenLog();
    void        		drawRefLog();
    void        		drawTraces();
    void        		drawMarker(FlatView::Annotation::AuxData*,
					    int,float,float,Color,bool);
    void        		drawUserPicks();
    void        		drawWellMarkers();
    void        		drawCShot();
    void        		fullRedraw();


protected:

    uiFlatViewer*		vwr_;

    ObjectSet<Well::Marker> 	markerset_;
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
    ObjectSet<SeisTrc>		trcs_;
    float			maxtraceval_;
    float			mintraceval_;

    ObjectSet<FlatView::Annotation::AuxData> userpickauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> wellmarkerauxdatas_;

    void        		initFlatViewer();
    void        		initLogViewers();
    void 			createVarDataPack(const char*,int);
    void 			setLogRanges(float,float);
    void 			setUpTrcBuf(SeisTrcBuf*,const char*,int);
    void			setUpUdfTrc(SeisTrc&,const char*,int);
    void			setUpValTrc(SeisTrc&,const char*,int);
    void        		setDataPack(SeisTrcBuf*,const char*,int);
    void 			removePacks(uiFlatViewer&);
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

