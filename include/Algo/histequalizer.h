#ifndef histequalizer_h
#define histequalizer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		June 2008
 RCS:		$Id: histequalizer.h,v 1.5 2012-08-03 13:00:04 cvskris Exp $
________________________________________________________________________

-*/

#include "algomod.h"
#include "ranges.h"
template <class T> class TypeSet;


mClass(Algo) HistEqualizer
{
public:
    			HistEqualizer(const int nrseg=256);
    void 		setData(const TypeSet<float>&);
    			//!< use in case of sorted data
    void 		setRawData(const TypeSet<float>&);
    			//!< use in case of unsorted data
    void 		update();
    float		position(float val) const;

protected:

    TypeSet<float>&	 	datapts_;
    const int		 	nrseg_;
    TypeSet<Interval<float> >*	histeqdatarg_;

    void		getSegmentSizes(TypeSet<int>&);
};

#endif


