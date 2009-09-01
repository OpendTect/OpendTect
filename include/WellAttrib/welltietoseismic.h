#ifndef welltietoseismic_h
#define welltietoseismic_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltietoseismic.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "ranges.h"

#include "welltied2tmodelmanager.h"
#include "welltiedata.h"
#include "welltieunitfactors.h"

template <class T> class Array1DImpl;
class BufferStringSet;
class DataPointSet;
class TaskRunner;
class Wavelet;
class WellTieSetup;
class WellTieD2TModelMGR;
class WellTieSynthetics;
class WellTieParams;

namespace Attrib { class DescSet; }
namespace Well 
{
    class Data;
    class D2TModel;
};

mClass WellTieToSeismic
{
public:
			WellTieToSeismic(WellTieDataHolder*,
					 const Attrib::DescSet&, 
					 TaskRunner*);
			~WellTieToSeismic();

    //TODO put back as private
    bool 		computeAll();
    bool		computeCrossCorrel();
    bool		computeSynthetics();
    bool		extractSeismics();
    bool		extractWellTrack();
    bool  		estimateWavelet();
    bool 	      	resampleLogs();
    void 	      	setWorkData();
    void 	      	createDispLogs();
   
    //D2TModelmanager operations
    void 		computeD2TModel()
			{ d2tmgr_->setFromVelLog(params_.currvellognm_); }
    bool 		saveD2TModel(const char* fname)
    			{ return d2tmgr_->save( fname ); }
    bool 		undoD2TModel()
			{ return d2tmgr_->undo(); }
    bool 		cancelD2TModel()
			{ return d2tmgr_->cancel(); }
    bool		updateD2TModel()
			{ return d2tmgr_->updateFromWD(); }
    bool		commitD2TModel()
			{ return d2tmgr_->commitToWD(); }
    
protected:

    TaskRunner*			tr_;      //becomes mine  

    DataPointSet* 		dps_;
    const Attrib::DescSet& 	ads_;
    Well::Data& 		wd_;
    WellTieData& 		wtdata_;     
    WellTieDataSetMGR& 		datamgr_;     
    WellTieDataSet& 		workdata_;
    WellTieDataSet& 		dispdata_;
    WellTieDataSet& 		corrdata_;
    const WellTieParams::DataParams& params_;	
    const WellTieSetup&		wtsetup_;	

    WellTieD2TModelMGR*		d2tmgr_;
    WellTieGeoCalculator*	geocalc_;
    WellTieSynthetics*		wtsynth_;
    
    void			checkShotCorr();
    void  			convolveWavelet();
    void 			reverseWavelet(Wavelet&);
    bool 	      		resLogExecutor(const char*);
};

#endif
