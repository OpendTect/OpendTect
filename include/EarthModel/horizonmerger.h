#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "paralleltask.h"
#include "emposid.h"
#include "enums.h"
#include "trckeysampling.h"

template <class T> class Array2D;

namespace EM
{

class Horizon3D;

/*!
\brief A subclass of ParallelTask that merges horizons.
*/

mExpClass(EarthModel) HorizonMerger : public ParallelTask
{
public:
    virtual		~HorizonMerger();

    enum Mode		{ Average, Top, Base };
			mDeclareEnumUtils(Mode)

    void		setMode( Mode m )	{ mode_ = m; }
    Mode		getMode() const		{ return mode_; }

protected:
			HorizonMerger();

    Mode		mode_		= Average;
};


/*!
\brief 3D HorizonMerger
*/

mExpClass(EarthModel) Horizon3DMerger : public HorizonMerger
{ mODTextTranslationClass(Horizon3DMerger)
public:
			Horizon3DMerger(const TypeSet<ObjectID>&);
			~Horizon3DMerger();

    Horizon3D*		getOutputHorizon();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

protected:
    od_int64		nrIterations() const override;

private:
    bool		doWork(od_int64 start,od_int64 stop,
					      int threadid) override;
    bool		doFinish(bool success) override;

    Array2D<float>*	depths_;
    Horizon3D*		outputhor_ = nullptr;
    ObjectSet<Horizon3D> inputhors_;

    TrcKeySampling	hs_;
    bool		ownsarray_ = true;
};

} // namespace EM
