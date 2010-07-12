#ifndef velocityvolumeconversation_h
#define velocityvolumeconversation_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityvolumeconversion.h,v 1.3 2010-07-12 22:52:41 cvskris Exp $
________________________________________________________________________


-*/

#include "cubesampling.h"
#include "task.h"
#include "thread.h"
#include "veldesc.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;

namespace Vel
{

/*!Reads in a volume with eather Vrms or Vint, and writes out a volume
   with eather Vrms or Vint. */

mClass VolumeConverter : public ParallelTask
{
public:
			VolumeConverter( IOObj* input,
					 IOObj* output,
					 const HorSampling& ranges,
					 const VelocityDesc& outdesc );
			~VolumeConverter();
    const char*		errMsg() const { return errmsg_.str(); }

    static const char*	sKeyInput();
    static const char*	sKeyOutput();

protected:
    od_int64		nrIterations() const { return hrg_.totalNr(); }
    bool		doPrepare(int);
    bool		doFinish(bool);
    bool		doWork(od_int64,od_int64,int);
    const char*		nrDoneText() const { return "Traces written"; }

    char		getNewTrace(SeisTrc&,int threadidx);
    bool		writeTraces();

    IOObj*		input_;
    IOObj*		output_;
    VelocityDesc	veldesc_;
    HorSampling		hrg_;
    BufferString	errmsg_;

    SeisTrcReader*	reader_;
    SeisTrcWriter*	writer_;
    ObjectSet<SeisTrc>	outputs_;
    int			maxbuffersize_;

    Threads::ConditionVar	lock_;
    TypeSet<int>	curtrcs_;
    int			getTrcIdx(const BinID&) const;
};



}; //namespace

#endif
