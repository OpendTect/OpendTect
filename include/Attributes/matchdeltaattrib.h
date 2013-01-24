#ifndef matchdeltaattrib_h
#define matchdeltaattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attribprovider.h"
template <class T> class ValueSeries;

namespace Attrib
{

/*!
\brief Computes the match delta between two cubes.
  Ties Max events to each other.
*/

mClass(Attributes) MatchDelta : public Provider
{
public:

    static void			initClass();
				MatchDelta(Desc&);

    static const char*		attribName()	   { return "MatchDelta"; }
    static const char*		maxshiftStr()	   { return "maxshift"; }

protected:

    static Provider*		createInstance(Desc&);

    float			maxshift_;
    Interval<int>		dessamps_;

    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,const BinID&,
	    				    int,int,int) const;
    const Interval<int>*	desZSampMargin(int,int) const;

    float			maxsamps_;
    const DataHolder*		refcubedata_;
    const DataHolder*		mtchcubedata_;
    ValueSeries<float>*		refseries_;
    ValueSeries<float>*		mtchseries_;
    mutable Interval<int>	refintv_;
    mutable Interval<int>	mtchintv_;

    mutable TypeSet<float>	deltas_;
    mutable TypeSet<float>	poss_;

    void			findEvents(int,int) const;
    void			fillOutput(const DataHolder&,int,int) const;

};

} // namespace Attrib


#endif
