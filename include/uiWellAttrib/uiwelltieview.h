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

template <class T> class Array1DImpl;
class WellTieSetup; 
class WellTieParams; 
class WellTieDataHolder; 
class WellTieDataSet; 
class WellTieDataSetMGR; 
class WellTiePickSet; 
class UserPick;
namespace Attrib { class DescSet; }

class uiFlatViewer;
class uiFunctionDisplay;
class uiLabel;
namespace Well
{
    class Data;
};
mClass uiWellTieView
{
public:
			    	uiWellTieView(uiParent*,WellTieDataHolder*);
				~uiWellTieView();


    const int 			viewerSize() const 	{ return vwrs_.size(); }
    uiFlatViewer* 		getViewer(int idx) 	{ return vwrs_[idx]; }
    const uiFlatViewer* 	getViewer(int idx) const { return vwrs_[idx]; }

    void        		fullRedraw();
    void        		createViewers(uiGroup*);
    void 			deleteUserPicks();
    void			deleteWellMarkers();
    void			deleteCheckShot();

    void        		drawAI();
    void        		drawVelLog();
    void        		drawDenLog();
    void        		drawMarker(FlatView::Annotation::AuxData*,
					    int,float,float,Color,bool);
    void        		drawReflectivity();
    void        		drawSynthetics();
    void        		drawSeismic();
    void        		drawUserPicks(const WellTiePickSet*);
    void        		drawWellMarkers();
    void        		drawCShot();

protected:

    WellTieDataSetMGR&		datamgr_; 		
    WellTieDataSet&  		data_;
    const Well::Data& 		wd_;		
    const WellTieSetup& 	wtsetup_;
    const WellTieParams& 	params_;
    ObjectSet<uiFlatViewer> 	vwrs_;

    bool 			isoriginalscale_;
    float			maxtraceval_;
    float			mintraceval_;

    ObjectSet<FlatView::Annotation::AuxData> userpickauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> wellmarkerauxdatas_;
    ObjectSet<FlatView::Annotation::AuxData> csauxdatas_;

    void        		initFlatViewer(const char*,int,int,int,bool,
						const Color&);
    void        		createVarDataPack(const char*,int,int);
    void 			removePacks(uiFlatViewer&);
    void			deleteMarkerAuxDatas(
				  ObjectSet<FlatView::Annotation::AuxData>&);
};



mClass uiWellTieCorrView : uiGroup
{
public:

				uiWellTieCorrView(uiParent*,WellTieDataHolder*);
				~uiWellTieCorrView();

    void                	setCrossCorrelation();

protected:

    uiLabel* 			corrlbl_;
    WellTieDataSet& 		data_;
    const WellTieParams& 	params_;
    ObjectSet<uiFunctionDisplay>  corrdisps_;
};

#endif

