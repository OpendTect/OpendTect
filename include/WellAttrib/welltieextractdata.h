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
#include "arrayndimpl.h"
#include "geometry.h"

class DataPointSet;
class BinID;
namespace Well 
{
    class Data;
    class Log;
};

namespace WellTie
{

class DataHolder;

mClass TrackExtractor : public Executor
{
public:
			TrackExtractor(DataPointSet* dps,const Well::Data* d)
			    : Executor("Extracting Well track positions")
			    , dps_(dps)
			    , wd_(*d)
			    , nrdone_(0)
			    , timeintv_(0,0,0)
			    {}

    StepInterval<float> timeintv_;

    int                 nextStep();
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }
    const BinID*	getBIDValues() const	{ return bidvalset_.arr(); }

protected:

    TypeSet<BinID>	bidvalset_;
    DataPointSet* 	dps_;
    const Well::Data& 	wd_;	 
    int                 nrdone_;
};



mClass LogResampler : public Executor
{
public:
			LogResampler(Well::Log*, const Well::Log&,
				const Well::Data*, WellTie::DataHolder* d=0);
			~LogResampler();



    int                 nextStep();
    int           	colnr_;
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }

    bool 		isavg_;
    Array1DImpl<float>* vals_;
    Array1DImpl<float>* dahs_;

    void		setTimeIntv(const StepInterval<float>&);

protected:

    Well::Log*   	newlog_;		
    const Well::Log& 	orglog_;
    const Well::Data& 	wd_;


    TypeSet<float> 	val_;
    TypeSet<float> 	dah_;

    int                 nrdone_;
    int                 curidx_;

    void        	fillProcLog(const Well::Log&);
    StepInterval<float> timeintv_;
};

};//namespace WellTie

#endif
