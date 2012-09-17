#ifndef prestackanglemutecomputer_h
#define prestackanglemutecomputer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          June 2011
 RCS:           $Id: prestackanglemutecomputer.h,v 1.3 2012/01/17 16:09:27 cvsbruno Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "ranges.h"
#include "horsampling.h"
#include "prestackanglemute.h"

class HorSampling;
class IOObj;
class MultiID;
class RayTracer1D;

namespace Vel { class VolumeFunctionSource; }

namespace PreStack
{
    class MuteDef;

mClass AngleMuteComputer : public ParallelTask, public AngleMuteBase
{
public:
    				AngleMuteComputer();
				~AngleMuteComputer();

    mStruct AngleMuteCompPars : public AngleMuteBase::Params
    {
	MultiID			outputmutemid_;
	HorSampling 		hrg_;
    };
    static const char*		sKeyMuteDefID() { return "Mute Def"; }

    void                	fillPar(IOPar&) const;
    bool                	usePar(const IOPar&);

    od_int64			nrIterations() const; 
    bool			doPrepare(int);
    bool			doWork(od_int64 start, od_int64 stop,int);
    bool			doFinish(bool success);

    const char*                 message() const { return "Computing mutes..."; }
    const char*			errMsg() const;

    AngleMuteCompPars&  	params();
    const AngleMuteCompPars&  	params() const;

protected:

    BufferString		errmsg_;

    MuteDef&                    outputmute_;

    Threads::Mutex              lock_;
};

}
#endif
