#ifndef seiszaxisstretcher_h
#define seiszaxisstretcher_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		January 2008
 RCS:		$Id: seiszaxisstretcher.h,v 1.5 2009-07-22 16:01:18 cvsbert Exp $
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

mClass SeisZAxisStretcher : public Executor
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
    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const			{ return totalnr_; }

    void		setLineKey(const char*);

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
    bool				is2d_;

    SeisTrc*				outtrc_;
    float*				outputptr_;

    int					nrdone_;
    int					totalnr_;
};

#endif
