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
class DataPointSet;
class WellTieSetup; 
class UserPick;

class uiFlatViewer;
namespace Attrib
{
    class DescSet;
}

namespace Well
{
    class Data;
};



mClass uiWellTieView
{
public:
			    	uiWellTieView(uiParent*,DataPointSet&,
				    ObjectSet< Array1DImpl<float> >&,
				    const Well::Data&, const WellTieSetup&,
				    const Attrib::DescSet&);
				~uiWellTieView();


    uiFlatViewer* 		getViewer(int idx) 	{ return vwrs_[idx]; }
    const uiFlatViewer* 	getViewer(int idx) const { return vwrs_[idx]; }
    
    const int 			viewerSize() const 	{ return vwrs_.size(); }

    void        		fullRedraw(uiGroup*);
    void        		createViewers(uiGroup*);
    void        		drawVelLog();
    void        		drawDensLog();
    void        		drawMarker(FlatView::Annotation::AuxData*,
					    int,float,float,Color,bool);
    void        		drawSynthetics();
    void        		drawSeismic();
    void        		drawUserPick(const UserPick*);
    void        		drawWellMarkers();
    void        		drawCShot();
    void        		setUpTimeAxis();

protected:

    ObjectSet< Array1DImpl<float> >&  dispdata_;
    DataPointSet&		dps_;
    const Well::Data& 		wd_;		
    const WellTieSetup& 	wtsetup_;
    const Attrib::DescSet& 	ads_;
    ObjectSet<uiFlatViewer> 	vwrs_;
    StepInterval<float>         timeintv_;
    ObjectSet<FlatView::Annotation::AuxData> userpickauxdatas_;

    void        		initFlatViewer(const char*,int,int,int,bool,
						const Color&);
    void        		createVarDataPack(const char*,int,int,int);
};

#endif

