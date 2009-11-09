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

class TaskRunner;

namespace WellTie
{
    class TrackExtractor;

mClass DataPlayer
{
public:
			DataPlayer(WellTie::DataHolder*,TaskRunner*);
			~DataPlayer();

    bool 		computeAll();
    bool 		computeWvltPack();
   
    //D2TModelmanager operations
    void 		computeD2TModel()
			{ d2tmgr_->setFromVelLog(params_.currvellognm_); }
    bool 		undoD2TModel()
			{ return d2tmgr_->undo(); }
    bool 		cancelD2TModel()
			{ return d2tmgr_->cancel(); }
    bool		updateD2TModel()
			{ return d2tmgr_->updateFromWD(); }
    bool		commitD2TModel()
			{ return d2tmgr_->commitToWD(); }
    
protected:

    TaskRunner*		tr_;      //becomes mine  

    Well::Data& 	wd_;
    Well::LogSet& 	logset_;

    const WellTie::Params::DataParams& params_;	
    const WellTie::Setup& wtsetup_;	
    WellTie::TrackExtractor *wtextr_;

    WellTie::DataHolder* dholder_;
    WellTie::D2TModelMGR* d2tmgr_;
    WellTie::GeoCalculator* geocalc_;
    ObjectSet<Wavelet>* wvltset_;

    bool		computeCrossCorrel();
    bool  		convolveWavelet();
    bool		extractSeismics();
    bool		extractWellTrack();
    bool  		estimateWavelet();
    void		checkShotCorr();
    bool		computeReflectivity();
    void 		getDPSZData(const DataPointSet&,Array1DImpl<float>&);
    bool 	      	resLogExecutor(const BufferStringSet&,bool,
				     const StepInterval<float>&);
    bool 	      	resampleLogs();
};

};//namespace WellTie
#endif
