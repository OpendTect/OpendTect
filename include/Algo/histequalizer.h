#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "ranges.h"

template <class T> class LargeValVec;
namespace Stats { class RandGen; }


/*!
\brief Class to do histogram equalization of datasets.
*/

mExpClass(Algo) HistEqualizer
{
public:
			HistEqualizer(const int nrseg=256);
			~HistEqualizer();

    void		setData(const LargeValVec<float>&);
			//!< use in case of sorted data
    void		setRawData(const TypeSet<float>&);
			//!< use in case of unsorted data
    void		update();
    float		position(float val) const;

protected:

    LargeValVec<float>&		datapts_;
    const int			nrseg_;
    TypeSet<Interval<float> >*	histeqdatarg_ = nullptr;
    Stats::RandGen&	gen_;

    void		getSegmentSizes(TypeSet<int>&);
};
