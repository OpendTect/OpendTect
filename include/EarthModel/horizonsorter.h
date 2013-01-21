#ifndef horizonsorter_h
#define horizonsorter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"

#include "cubesampling.h"
#include "multiid.h"
#include "position.h"
#include "surv2dgeom.h"


namespace EM { class Horizon; }
template <class T> class Array3D;
class HorSamplingIterator;
class BufferStringSet;

/*!
\brief Executor to sort horizons.
*/

mExpClass(EarthModel) HorizonSorter : public Executor
{
public:

				HorizonSorter(const TypeSet<MultiID>&,
					      bool is2d=false);
				~HorizonSorter();

    void			getSortedList(TypeSet<MultiID>&);
    const HorSampling&		getBoundingBox() const	{ return hrg_; }
    int				getNrCrossings(const MultiID&,
	    				       const MultiID&) const;

    const char*			message() const;
    od_int64			totalNr() const;
    od_int64			nrDone() const;
    const char*			nrDoneText() const;

protected:

    int				nextStep();
    void			calcBoundingBox();
    void			init();
    void			sort();

    int				totalnr_;
    int				nrdone_;

    bool			is2d_;
    TypeSet<PosInfo::GeomID>	geomids_;
    TypeSet<StepInterval<int> >	trcrgs_;

    HorSamplingIterator*	iterator_;
    BinID			binid_;
    HorSampling			hrg_;
    ObjectSet<EM::Horizon>	horizons_;
    Array3D<int>*		result_;
    TypeSet<MultiID>		unsortedids_;
    TypeSet<MultiID>		sortedids_;

    BufferString		message_;
};


#endif

