#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		June 2008
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

