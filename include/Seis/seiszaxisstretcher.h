#ifndef seiszaxisstretcher_h
#define seiszaxisstretcher_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		January 2008
 RCS:		$Id: seiszaxisstretcher.h,v 1.1 2008-01-04 22:45:59 cvskris Exp $
________________________________________________________________________

*/


#include "cubesampling.h"
#include "executor.h"

class IOObj;
class ZAxisTransform;
class SeisTrcReader;
class SeisTrcWriter;
class ZAxisTransformSampler;
class ZAxisTransform;
class SeisTrc;


/*!Stretches the zaxis from the input cube with a ZAxisTransform and writes it
   out into another volume. */

class SeisZAxisStretcher : public Executor
{
public:
    			SeisZAxisStretcher( const IOObj& in,
					     const IOObj& out,
					     const CubeSampling& outcs,
					     ZAxisTransform&,
					     bool forward);
			~SeisZAxisStretcher();

    bool		isOK() const;

    int			nextStep();
    int			nrDone() const			{ return nrdone_; }
    int			totalNr() const			{ return totalnr_; }

protected:

    void				nextChunk();

    SeisTrcReader*			seisreader_;
    SeisTrcWriter*			seiswriter_;
    CubeSampling			outcs_;
    HorSampling				curhrg_;
    ZAxisTransform&			ztransform_;
    int 				voiid_;
    ZAxisTransformSampler*		sampler_;
    bool				forward_;

    SeisTrc*				outtrc_;
    float*				outputptr_;

    int					nrdone_;
    int					totalnr_;
};

#endif
