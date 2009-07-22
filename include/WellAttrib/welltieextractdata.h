#ifndef welltieextractdata_h
#define welltieextractdata_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltieextractdata.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "executor.h"
#include "geometry.h"

class DataPointSet;
class WellTieGeoCalculator;
class WellTieDataSet;
namespace Well 
{
    class Data;
    class Log;
};

mClass WellTieExtractTrack : public Executor
{
public:
			WellTieExtractTrack(DataPointSet&,const Well::Data*);
			~WellTieExtractTrack() {};


    StepInterval<double> timeintv_;

    int                 nextStep();
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }

protected:

    DataPointSet& 	dps_;
    const Well::Data& 	wd_;	 
    int                 nrdone_;
};



mClass WellTieResampleLog : public Executor
{
public:
			WellTieResampleLog(WellTieDataSet&,const Well::Log&,
					const Well::Data*,WellTieGeoCalculator&);
			~WellTieResampleLog() {};


    StepInterval<double> timeintv_;

    int                 nextStep();
    int           	colnr_;
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }
    const char* 	dptnm_;
    const char* 	timenm_;


protected:

    WellTieDataSet&     workdata_;		
    const char* 	logname_;
    TypeSet<float> 	val_;
    TypeSet<float> 	dah_;
    const Well::Data& 	wd_;
    int                 nrdone_;
    int                 curlogsample_;

    void      	 	updateLogIdx(float,int&);
    void        	fillProcLog(const Well::Log&);
};


#endif
