#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		January 2008
________________________________________________________________________

*/


#include "seismod.h"
#include "trckeyzsampling.h"
#include "paralleltask.h"
#include "thread.h"
#include "unitofmeasure.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;
class ZAxisTransform;


/*!Stretches the zaxis from the input cube with a ZAxisTransform and writes it
   out into another volume. If stretchinverse is true, the stretching will
   be done on the inveres of the values. */

mExpClass(Seis) SeisZAxisStretcher : public ParallelTask
{ mODTextTranslationClass(SeisZAxisStretcher);
public:
			SeisZAxisStretcher( const IOObj& in,
					     const IOObj& out,
					     const TrcKeyZSampling& outcs,
					     ZAxisTransform&,
					     bool forward,
					     bool stretchz);
			~SeisZAxisStretcher();

    bool		isOK() const;

    void		setGeomID(Pos::GeomID);
    uiString		uiMessage() const override
			{ return tr("Stretching data"); }

    void		setVelTypeIsVint( bool yn )	{ isvint_ = yn; }
    void		setVelTypeIsVrms( bool yn )	{ isvrms_ = yn; }

protected:

    void		init(const IOObj& in,const IOObj& out);
    bool		doPrepare(int) override;
    bool		doFinish(bool) override;
    bool		doWork(od_int64,od_int64,int) override;
    od_int64		nrIterations() const override	{ return totalnr_; }

    bool				getInputTrace(SeisTrc&,TrcKey&);
    bool				getModelTrace(SeisTrc&,TrcKey&);
    bool				loadTransformChunk(int firstinl);

    SeisTrcReader*			seisreader_;
    Threads::ConditionVar		readerlock_;
    Threads::ConditionVar		readerlockmodel_;

    SeisTrcReader*			seisreadertdmodel_;

    SeisTrcWriter*			seiswriter_;
    SeisSequentialWriter*		sequentialwriter_;
    bool				waitforall_;
    int					nrwaiting_;
    int					nrthreads_;

    TrcKeyZSampling			outcs_;
    TrcKeySampling			curhrg_;
    ZAxisTransform*			ztransform_;
    int					voiid_;
    bool				ist2d_;
    bool				is2d_;
    bool				stretchz_;

    int					totalnr_;
    bool				isvint_;
    bool				isvrms_;

};

