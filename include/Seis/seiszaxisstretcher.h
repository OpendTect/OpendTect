#ifndef seiszaxisstretcher_h
#define seiszaxisstretcher_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		January 2008
 RCS:		$Id: seiszaxisstretcher.h,v 1.13 2012-08-03 13:00:39 cvskris Exp $
________________________________________________________________________

*/


#include "seismod.h"
#include "cubesampling.h"
#include "task.h"

class IOObj;
class MultiID;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;
class ZAxisTransform;


/*!Stretches the zaxis from the input cube with a ZAxisTransform and writes it
   out into another volume. If stretchinverse is true, the stretching will
   be done on the inveres of the values. */

mClass(Seis) SeisZAxisStretcher : public ParallelTask
{
public:
    			SeisZAxisStretcher( const IOObj& in,
					     const IOObj& out,
					     const CubeSampling& outcs,
					     ZAxisTransform&,
					     bool forward,
					     bool stretchz);
    			SeisZAxisStretcher( const IOObj& in,
					     const IOObj& out,
					     const CubeSampling& outcs,
					     const MultiID& tdmodelmid,
					     bool forward,
					     bool stretchz );
			~SeisZAxisStretcher();

    bool		isOK() const;

    void		setLineKey(const char*);
    const char*		message() const { return "Stretching data"; }

    void		setVelTypeIsVint( bool yn )	{ isvint_ = yn; }
    void		setVelTypeIsVrms( bool yn )	{ isvrms_ = yn; }

protected:

    void		init(const IOObj& in,const IOObj& out);
    bool		doPrepare(int);
    bool		doFinish(bool);
    bool		doWork(od_int64,od_int64,int);
    od_int64		nrIterations() const		{ return totalnr_; }

    bool				getInputTrace(SeisTrc&,BinID&);
    bool				getModelTrace(SeisTrc&,BinID&);
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

    CubeSampling			outcs_;
    HorSampling				curhrg_;
    ZAxisTransform*			ztransform_;
    int 				voiid_;
    bool				ist2d_;
    bool				is2d_;
    bool				stretchz_;

    int					totalnr_;
    bool				isvint_;
    bool				isvrms_;

};

#endif

