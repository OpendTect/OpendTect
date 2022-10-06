#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "binid.h"
#include "emobject.h"
#include "executor.h"
#include "multiid.h"
#include "posinfo2dsurv.h"
#include "trckeyzsampling.h"


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
				HorizonSorter(const TypeSet<EM::ObjectID>&,
					      bool is2d=false);
				~HorizonSorter();

    void			setTaskRunner(TaskRunner&);

    void			getSortedList(TypeSet<MultiID>&);
    const TrcKeySampling&		getBoundingBox() const	{ return tks_; }
    int				getNrCrossings(const MultiID&,
					       const MultiID&) const;

    void			getSortedList(TypeSet<EM::ObjectID>&);
    int				getNrCrossings(const EM::ObjectID,
					       const EM::ObjectID) const;

    uiString			uiMessage() const override;
    od_int64			totalNr() const override;
    od_int64			nrDone() const override;
    uiString			uiNrDoneText() const override;

protected:

    int				nextStep() override;
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
    TrcKeySampling		tks_;
    ObjectSet<EM::Horizon>	horizons_;
    Array3D<int>*		result_;
    TypeSet<EM::ObjectID>	unsortedids_;
    TypeSet<EM::ObjectID>	sortedids_;
    TypeSet<MultiID>		unsortedkeys_;
    TypeSet<MultiID>		sortedkeys_;
    TaskRunner*			taskrun_;

    uiString			message_;
};
