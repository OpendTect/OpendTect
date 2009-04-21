#ifndef welltietoseismic_h
#define welltietoseismic_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltietoseismic.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "ranges.h"

#include "welltied2tmodelmanager.h"

template <class T> class Array1DImpl;
class BufferStringSet;
class DataPointSet;
class TaskRunner;
class Wavelet;
class WellTieSetup;
class WellTieD2TModelManager;
class WellTieCSCorr;
class WellTieSynthetics;

namespace Attrib { class DescSet; }
namespace Well 
{
    class Data;
    class D2TModel;
};

mClass WellTieToSeismic
{
public:
			WellTieToSeismic(const WellTieSetup&,
					 const Attrib::DescSet&,DataPointSet&,
					 ObjectSet< Array1DImpl<float> >&,
					 Well::Data&,TaskRunner*);
			~WellTieToSeismic();


    
    bool 			computeAll();
    bool			computeSynthetics();
    Wavelet*  			estimateWavelet();
   
    //D2TModelmanager 
    void 			computeD2TModel()
				{ d2tmgr_->setFromVelLog(); }
    bool 			saveD2TModel(const char* fname)
    				{ return d2tmgr_->save( fname ); }
    bool 			undoD2TModel()
				{ return d2tmgr_->undo(); }
    bool 			cancelD2TModel()
				{ return d2tmgr_->commitToWD(); }
    bool			updateD2TModel()
				{ return d2tmgr_->updateFromWD(); }
    bool			commitD2TModel()
				{ return d2tmgr_->commitToWD(); }
    
protected:

    TaskRunner*			tr_;      //becomes mine  

    const Attrib::DescSet& 	ads_;
    const WellTieSetup&		wtsetup_;	
    Well::Data& 		wd_;	 
    StepInterval<float>   	timeintv_;
    DataPointSet& 		dps_;
    ObjectSet< Array1DImpl<float> >   workdata_;	
    ObjectSet< Array1DImpl<float> >&  dispdata_;	

    WellTieD2TModelManager*	d2tmgr_;
    WellTieGeoCalculator*	geocalc_;
    WellTieCSCorr*		cscorr_;
    WellTieSynthetics*		wtsynth_;
    
    bool			extractSeismics();
    bool			extractWellTrack();
    bool 	      		resampleLogs();
    bool 	      		resLogExecutor(const char*,int);
    bool 	      		createDPSCols();
    void	 	   	fillDispData();
    void			checkShotCorr();
    void 			sortDPSDataAlongZ();
    bool 	      		setLogsParams();
};

#endif
