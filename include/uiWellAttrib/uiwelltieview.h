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
class WellTieDataSet; 
class WellTieDataMGR; 
class UserPick;
namespace Attrib { class DescSet; }

class uiFlatViewer;
namespace Well
{
    class Data;
};
mClass uiWellTieView
{
public:
			    	uiWellTieView(uiParent*, WellTieDataMGR&,
				    const Well::Data*, const WellTieParams*,
				    const Attrib::DescSet& ads );
				~uiWellTieView();


    const int 			viewerSize() const 	{ return vwrs_.size(); }
    uiFlatViewer* 		getViewer(int idx) 	{ return vwrs_[idx]; }
    const uiFlatViewer* 	getViewer(int idx) const { return vwrs_[idx]; }

    void        		fullRedraw();
    void        		createViewers(uiGroup*);
    void			deleteMarkerAuxDatas(const char*);
    void        		drawVelLog();
    void        		drawDenLog();
    void        		drawMarker(FlatView::Annotation::AuxData*,
					    int,float,float,Color,bool);
    void        		drawReflectivity();
    void        		drawSynthetics();
    void        		drawSeismic();
    void        		drawUserPicks(const ObjectSet<UserPick>&);
    void        		drawWellMarkers();
    void        		drawCShot();

protected:

    WellTieDataMGR&		datamgr_; 		
    WellTieDataSet&  		data_;
    const Well::Data& 		wd_;		
    const WellTieSetup& 	wtsetup_;
    const WellTieParams& 	params_;
    const Attrib::DescSet& 	ads_;
    ObjectSet<uiFlatViewer> 	vwrs_;
    ObjectSet<FlatView::Annotation::AuxData> userpickauxdatas_;

    void        		initFlatViewer(const char*,int,int,int,bool,
						const Color&);
    void        		createVarDataPack(const char*,int,int);
};

#endif

