#ifndef seiszaxisstretcher_h
#define seiszaxisstretcher_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		January 2008
 RCS:		$Id: seiszaxisstretcher.h,v 1.9 2011-03-21 01:22:06 cvskris Exp $
________________________________________________________________________

*/


#include "cubesampling.h"
#include "task.h"

class IOObj;
class ZAxisTransform;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;
class ZAxisTransform;
class SeisTrc;


/*!Stretches the zaxis from the input cube with a ZAxisTransform and writes it
   out into another volume. If stretchinverse is true, the stretching will
   be done on the inveres of the values. */

mClass SeisZAxisStretcher : public ParallelTask
{
public:
    			SeisZAxisStretcher( const IOObj& in,
					     const IOObj& out,
					     const CubeSampling& outcs,
					     ZAxisTransform&,
					     bool forward,
					     bool stretchinverse);
			~SeisZAxisStretcher();

    bool		isOK() const;

    void		setLineKey(const char*);
    const char*		message() const { return "Stretching data"; }

protected:

    bool		doPrepare(int);
    bool		doFinish(bool);
    bool		doWork(od_int64,od_int64,int);
    od_int64		nrIterations() const		{ return totalnr_; }

    bool				getTrace(SeisTrc&,BinID&);
    bool				loadTransformChunk(int firstinl);

    SeisTrcReader*			seisreader_;
    Threads::ConditionVar		readerlock_;

    SeisTrcWriter*			seiswriter_;
    SeisSequentialWriter*		sequentialwriter_;
    bool				waitforall_;
    int					nrwaiting_;
    int					nrthreads_;

    CubeSampling			outcs_;
    HorSampling				curhrg_;
    ZAxisTransform&			ztransform_;
    int 				voiid_;
    bool				forward_;
    bool				is2d_;
    bool				stretchinverse_;

    int					totalnr_;

};

#endif
