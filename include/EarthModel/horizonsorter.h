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

#include "trckeyzsampling.h"
#include "multiid.h"
#include "binid.h"
#include "posinfo2dsurv.h"


namespace EM { class Horizon; }
template <class T> class Array3D;
class TrcKeySamplingIterator;

/*!
\brief Executor to sort horizons.
*/

mExpClass(EarthModel) HorizonSorter : public Executor
{ mODTextTranslationClass(HorizonSorter);
public:

				HorizonSorter(const TypeSet<MultiID>&,
					      bool is2d=false);
				~HorizonSorter();

    void			setTaskRunner(TaskRunner&);

    void			getSortedList(TypeSet<MultiID>&);
    const TrcKeySampling&		getBoundingBox() const	{ return tks_; }
    int				getNrCrossings(const MultiID&,
					       const MultiID&) const;

    uiString			uiMessage() const;
    od_int64			totalNr() const;
    od_int64			nrDone() const;
    uiString			uiNrDoneText() const;

protected:

    int				nextStep();
    void			calcBoundingBox();
    void			init();
    void			sort();

    int				totalnr_;
    int				nrdone_;

    bool			is2d_;
    TypeSet<Pos::GeomID>	geomids_;
    TypeSet<StepInterval<int> >	trcrgs_;

    TrcKeySamplingIterator*	iterator_;
    BinID			binid_;
    TrcKeySampling			tks_;
    ObjectSet<EM::Horizon>	horizons_;
    Array3D<int>*		result_;
    TypeSet<MultiID>		unsortedids_;
    TypeSet<MultiID>		sortedids_;
    TaskRunner*			taskrun_;

    uiString			message_;
};


#endif
