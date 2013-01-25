#ifndef zaxistransformer_h
#define zaxistransformer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "algomod.h"
#include "task.h"
#include "cubesampling.h"

class TaskRunner;
class ZAxisTransform;

template <class T> class Array3D;

/*!
\brief Transforms an Array3D with a ZAxisTransform. It is assumed that the
first dimension in the array is inline, the second is crossline and that the
third is z.
*/

mExpClass(Algo) ZAxisTransformer : public ParallelTask
{
public:
    			ZAxisTransformer(ZAxisTransform&,bool forward = true);
			~ZAxisTransformer();
    void		setInterpolate(bool yn);
    bool		getInterpolate() const;
    bool		setInput(const Array3D<float>&,const CubeSampling&);
    void		setOutputRange(const CubeSampling&);
    const CubeSampling&	getOutputRange() const	{ return outputcs_; }
    Array3D<float>*	getOutput(bool transfer);
    			/*!<\param transfer specifies whether the caller will
			                    take over the array.  */
    bool		loadTransformData(TaskRunner* =0);

    int			getVoiID() const		{ return voiid_; }
    void		removeVoiOnDelete( bool yn )	{ rmvoi_ = yn; }

protected:
    bool		doPrepare(int);
    od_int64		nrIterations() const;
    bool		doWork( od_int64, od_int64, int );

    ZAxisTransform&		transform_;
    int				voiid_;
    bool			forward_;
    bool			interpolate_;
    bool			rmvoi_;

    const Array3D<float>*	input_;
    CubeSampling		inputcs_;

    Array3D<float>*		output_;
    CubeSampling		outputcs_;
};



#endif

