#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "trckeyzsampling.h"
#include "thread.h"
#include "paralleltask.h"
#include "veldesc.h"
#include "uistring.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;

namespace Vel
{

/*!Reads in a volume with eather Vrms or Vint, and writes out a volume
   with eather Vrms or Vint. */

mExpClass(Velocity) VolumeConverter : public ParallelTask
{ mODTextTranslationClass(VolumeConverter);
public:
				VolumeConverter(const IOObj& input,
						const IOObj& output,
						const TrcKeySampling& ranges,
						const VelocityDesc& outdesc);
				~VolumeConverter();

    uiString			errMsg() const { return errmsg_; }

    static const char*		sKeyInput();
    static const char*		sKeyOutput();

protected:
    od_int64			nrIterations() const { return totalnr_; }
    bool			doPrepare(int);
    bool			doFinish(bool);
    bool			doWork(od_int64,od_int64,int);
    uiString			uiNrDoneText() const { 
						return tr("Traces written");
						     }

    char			getNewTrace(SeisTrc&,int threadidx);

    od_int64			totalnr_;
    IOObj*			input_;
    IOObj*			output_;
    VelocityDesc		velinpdesc_;
    VelocityDesc		veloutpdesc_;
    TrcKeySampling			tks_;
    uiString			errmsg_;

    SeisTrcReader*		reader_;
    SeisTrcWriter*		writer_;
    SeisSequentialWriter*	sequentialwriter_;

    Threads::ConditionVar	lock_;
};

} // namespace Vel

