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

namespace Well { class Data; class ExtractParams; }

mClass LogCubeCreator : public ParallelTask
{
public:
				LogCubeCreator(const Well::Data&);
				~LogCubeCreator();


    mStruct LogCubeData
    {
				LogCubeData(const BufferString& l,CtxtIOObj& c)
				    : seisctio_(c), lognm_(l) {}
			        ~LogCubeData();	

	CtxtIOObj& 		seisctio_;  
	const BufferString&	lognm_;
    };

    void			setInput(ObjectSet<LogCubeData>&,int nrtrcs);
    				//LogCubeDatas become mine

    const char* 		errMsg() const;

    od_int64    		totalNr() const { return nrdone_; };

protected:

    const Well::Data&		wd_;
    BufferString 		errmsg_;
    TypeSet<BinID>		binids_;
    HorSampling			hrg_;
    int				nrduplicatetrcs_;
    ObjectSet<LogCubeData>	logdatas_;

    od_int64                    nrIterations() const { return logdatas_.size();}
    od_int64            	nrdone_;
    
    bool			doPrepare(int);
    bool 			doWork(od_int64,od_int64,int);

    bool                        writeLog2Cube(const LogCubeData&) const;

public:
    void			setInput(ObjectSet<LogCubeData>&,int nrtrcs,
	    				const Well::ExtractParams&);
    				//LogCubeDatas become mine
};

#endif
