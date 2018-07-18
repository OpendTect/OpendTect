#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

#include "emcommon.h"
#include "paralleltask.h"
#include "enums.h"
#include "trckeysampling.h"

template <class T> class Array2D;

namespace EM
{

class Horizon3D;

/*!\brief A ParallelTask that merges horizons. */

mExpClass(EarthModel) HorizonMerger : public ParallelTask
{
public:
    enum Mode		{ Average, Top, Base };
			mDeclareEnumUtils(Mode)

    void		setMode( Mode m )	{ mode_ = m; }
    Mode		getMode() const		{ return mode_; }

protected:
			HorizonMerger()	: mode_(Average)	{}

    Mode		mode_;
};


/*!\brief 3D HorizonMerger */

mExpClass(EarthModel) Horizon3DMerger : public HorizonMerger
{
public:
			Horizon3DMerger(const DBKeySet&);
			~Horizon3DMerger();

    Horizon3D*		getOutputHorizon();

protected:
    od_int64		nrIterations() const;

private:
    bool		doWork(od_int64 start,od_int64 stop,int threadid);
    bool		doFinish(bool success);

    Array2D<float>*	depths_;
    Horizon3D*		outputhor_;
    ObjectSet<Horizon3D> inputhors_;

    TrcKeySampling	hs_;
    bool		ownsarray_;
};

} // namespace EM
