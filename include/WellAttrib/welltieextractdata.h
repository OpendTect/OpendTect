#ifndef welltieextractdata_h
#define welltieextractdata_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltieextractdata.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "executor.h"
#include "geometry.h"

template <class T> class Array1DImpl;
class DataPointSet;
namespace Well 
{
    class Data;
    class Log;
};

mClass WellTieExtractTrack : public Executor
{
public:
			WellTieExtractTrack(DataPointSet&,const Well::Data&);
			~WellTieExtractTrack() {};


    StepInterval<float> timeintv_;

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
			WellTieResampleLog(ObjectSet< Array1DImpl<float> >&,
					   const Well::Log&,
					   const Well::Data&);
			~WellTieResampleLog() {};


    StepInterval<float> timeintv_;

    int                 nextStep();
    int           	colnr_;
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }


protected:

    ObjectSet< Array1DImpl<float> >  workdata_;		
    const char* 	logname_;
    TypeSet<float> 	val_;
    TypeSet<float> 	dah_;
    const Well::Data& 	wd_;
    int                 nrdone_;

    float      	 	findNearestLogSample(const float);
    void        	fillProcLog(const Well::Log&);
    int         	getFirstDefIdx(const TypeSet<float>&);
    int         	getLastDefIdx(const TypeSet<float>&);
    void        	interpolateData(TypeSet<float>&,const float,const bool);
    bool        	isValidLogData(const TypeSet<float>&);
};


#endif
