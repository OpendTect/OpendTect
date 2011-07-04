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

#include "task.h"
#include "horsampling.h"

class BinID;
class CtxtIOObj;

namespace Well { class Track; class D2TModel; class Log;  }

mClass LogCubeCreator : public ParallelTask
{
public:
				LogCubeCreator(const Well::Track&,
						const Well::D2TModel*);
				~LogCubeCreator();


    mStruct LogCubeData
    {
				LogCubeData(const Well::Log& l, CtxtIOObj& ctio)
				    : seisctio_(ctio), log_(l) {}
			        ~LogCubeData();	

	CtxtIOObj& 		seisctio_;  
	const Well::Log&	log_;
    };

    void			setInput(ObjectSet<LogCubeData>&,int nrtrcs);
    				//LogCubeDatas become mine

    const char* 		errMsg() const;

protected:

    const Well::D2TModel*	d2t_;

    BufferString 		errmsg_;
    TypeSet<BinID>		binids_;
    HorSampling			hrg_;
    int				nrduplicatetrcs_;
    ObjectSet<LogCubeData>	logdatas_;

    od_int64                    nrIterations() const { return logdatas_.size();}
    bool			doPrepare(int);
    bool 			doWork(od_int64,od_int64,int);

    bool                        writeLog2Cube(const LogCubeData&) const;
};

#endif
