#ifndef createlogcube_h
#define createlogcube_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          July 2011
RCS:           $Id: createlogcube.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________
-*/

#include "wellattribmod.h"
#include "task.h"
#include "horsampling.h"
#include "wellextractdata.h"

class BinID;
class CtxtIOObj;

namespace Well { class Data;  }

mExpClass(WellAttrib) LogCubeCreator : public ParallelTask
{
public:
				LogCubeCreator(const Well::Data&);
				~LogCubeCreator();
    mStruct(WellAttrib) LogCubeData
    {
				LogCubeData(const char* log,CtxtIOObj& c)
				    : seisctio_(c), lognm_(log) {}
			        ~LogCubeData();	

	CtxtIOObj& 		seisctio_;  
	BufferString		lognm_;
    };

				//LogCubeDatas become mine
    void			setInput(ObjectSet<LogCubeData>&,int nrtrcs);
    void			setInput(ObjectSet<LogCubeData>&,int nrtrcs,
	    				const Well::ExtractParams&);

    const char* 		errMsg() const;

    od_int64    		totalNr() const { return nrdone_; };

protected:

    const Well::Data&		wd_;
    BufferString 		errmsg_;
    TypeSet<BinID>		binids_;
    HorSampling			hrg_;
    int				nrduplicatetrcs_;
    ObjectSet<LogCubeData>	logdatas_;
    Well::ExtractParams		extractparams_;

    od_int64                    nrIterations() const { return logdatas_.size();}
    od_int64            	nrdone_;
    
    bool			doPrepare(int);
    bool 			doWork(od_int64,od_int64,int);

    bool                        writeLog2Cube(const LogCubeData&) const;
};

#endif


