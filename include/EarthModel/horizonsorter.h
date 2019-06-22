#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		April 2006
________________________________________________________________________

-*/

#include "emcommon.h"
#include "executor.h"
#include "trckeysampling.h"
#include "dbkey.h"
#include "binid.h"
#include "posinfo2dsurv.h"


namespace EM { class Horizon; }
template <class T> class Array3D;
class TrcKeySamplingIterator;

/*!\brief Executor to sort horizons. */

mExpClass(EarthModel) HorizonSorter : public Executor
{ mODTextTranslationClass(HorizonSorter);
public:

				HorizonSorter(const DBKeySet&,
					      const TaskRunnerProvider&,
					      bool is2d=false);
				~HorizonSorter();

    void			getSortedList(DBKeySet&);
    const TrcKeySampling&		getBoundingBox() const	{ return tks_; }
    int				getNrCrossings(const DBKey&,
					       const DBKey&) const;

    uiString			message() const;
    od_int64			totalNr() const;
    od_int64			nrDone() const;
    uiString			nrDoneText() const;

protected:

    int				nextStep();
    void			calcBoundingBox();
    void			init();
    void			sort();

    int				totalnr_;
    int				nrdone_;

    bool			is2d_;
    GeomIDSet			geomids_;
    TypeSet<StepInterval<int> >	trcrgs_;

    TrcKeySamplingIterator*	iterator_;
    BinID			binid_;
    TrcKeySampling		tks_;
    ObjectSet<EM::Horizon>	horizons_;
    Array3D<int>*		result_;
    DBKeySet			unsortedids_;
    DBKeySet			sortedids_;
    const TaskRunnerProvider&	trprov_;

    uiString			message_;

};
