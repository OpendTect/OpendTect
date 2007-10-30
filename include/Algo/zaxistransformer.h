#ifndef zaxistransformer_h
#define zaxistransformer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2007
 RCS:           $Id: zaxistransformer.h,v 1.3 2007-10-30 16:53:35 cvskris Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "cubesampling.h"

class ZAxisTransform;

template <class T> class Array3D;

/*!Transforms an Array3D with a zaxistransform. It is assumed that the first
   dimension in the array is inline, the second is crossline and that the third
   is z.
*/


class ZAxisTransformer : public ParallelTask
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
    bool		loadTransformData();

protected:
    bool		doPrepare(int);
    int			totalNr() const;
    bool		doWork( int, int, int );

    ZAxisTransform&		transform_;
    int				voiid_;
    bool			forward_;
    bool			interpolate_;

    const Array3D<float>*	input_;
    CubeSampling		inputcs_;

    Array3D<float>*		output_;
    CubeSampling		outputcs_;
};



#endif
