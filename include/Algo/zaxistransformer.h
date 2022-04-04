#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/

#include "algomod.h"
#include "paralleltask.h"
#include "trckeyzsampling.h"

class TaskRunner;
class ZAxisTransform;

template <class T> class Array3D;

/*!
\brief Transforms an Array3D with a ZAxisTransform. It is assumed that the
first dimension in the array is inline, the second is crossline and that the
third is z.
*/

mExpClass(Algo) ZAxisTransformer : public ParallelTask
{ mODTextTranslationClass(ZAxisTransformer);
public:
			ZAxisTransformer(ZAxisTransform&,bool forward = true);
			~ZAxisTransformer();
    void		setInterpolate(bool yn);
    bool		getInterpolate() const;
    bool		setInput(const Array3D<float>&,const TrcKeyZSampling&);
    void		setOutputRange(const TrcKeyZSampling&);
    const TrcKeyZSampling&	getOutputRange() const	{ return outputcs_; }
    Array3D<float>*	getOutput(bool transfer);
			/*!<\param transfer specifies whether the caller will
			                    take over the array.  */
    bool		loadTransformData(TaskRunner* =0);

    int			getVoiID() const		{ return voiid_; }
    void		removeVoiOnDelete( bool yn )	{ rmvoi_ = yn; }

protected:
    bool		doPrepare(int) override;
    od_int64		nrIterations() const override;
    bool		doWork(od_int64,od_int64,int) override;

    ZAxisTransform&		transform_;
    int				voiid_;
    bool			forward_;
    bool			interpolate_;
    bool			rmvoi_;

    const Array3D<float>*	input_;
    TrcKeyZSampling		inputcs_;

    Array3D<float>*		output_;
    TrcKeyZSampling		outputcs_;

public:
    uiString		uiMessage() const override
			{ return tr("Z-axis transform");  }
    uiString		uiNrDoneText() const override
			{ return ParallelTask::sTrcFinished(); }
};



