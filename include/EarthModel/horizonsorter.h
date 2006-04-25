#ifndef horizonsorter_h
#define horizonsorter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id: horizonsorter.h,v 1.1 2006-04-25 15:20:33 cvsnanne Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "multiid.h"
#include "position.h"

namespace EM { class Horizon; }
template <class T> class Array3D;
class HorSamplingIterator;

class HorizonSorter : public Executor
{
public:

				HorizonSorter();
				~HorizonSorter();

    void			addHorizons(const TypeSet<MultiID>&);
    void			getSortedList(TypeSet<MultiID>&);

    const char*			message() const;
    int				totalNr() const;
    int				nrDone() const;
    const char*			nrDoneText() const;

protected:

    int				nextStep();
    void			calcBoundingBox();
    void			sort();

    int				totalnr_;
    int				nrdone_;

    HorSamplingIterator*	iterator_;
    BinID			binid_;
    ObjectSet<EM::Horizon>	horizons_;
    Array3D<int>*		result_;
    TypeSet<MultiID>		unsortedids_;
    TypeSet<MultiID>		sortedids_;
};


#endif
