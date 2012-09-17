#ifndef horizonmerger_h
#define horizonmerger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: horizonmerger.h,v 1.2 2011/05/20 11:47:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "emposid.h"
#include "enums.h"
#include "horsampling.h"

template <class T> class Array2D;

namespace EM
{

class Horizon3D;

mClass HorizonMerger : public ParallelTask
{
public:
    enum Mode		{ Average, Top, Base };
			DeclareEnumUtils(Mode)

    void		setMode( Mode m )	{ mode_ = m; }
    Mode		getMode() const		{ return mode_; }

protected:
			HorizonMerger()	: mode_(Average)	{}

    Mode		mode_;
};


mClass Horizon3DMerger : public HorizonMerger
{
public:
			Horizon3DMerger(const TypeSet<ObjectID>&);
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

    HorSampling		hs_;
    bool		ownsarray_;
};

} // namespace EM

#endif

