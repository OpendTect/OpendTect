#ifndef histequalizer_h
#define histequalizer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		June 2008
 RCS:		$Id: histequalizer.h,v 1.4 2009/07/22 16:01:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
template <class T> class TypeSet;


mClass HistEqualizer
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

