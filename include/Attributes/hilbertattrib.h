#ifndef hilbertattrib_h
#define hilbertattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: hilbertattrib.h,v 1.9 2006-10-18 12:19:58 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

namespace Attrib
{

/*!\brief Hilbert attribute.

  Calculates Hilbert transform
*/


class Hilbert : public Provider
{
public:
    static void			initClass();
				Hilbert(Desc&);
				~Hilbert()	{ delete [] hilbfilter_; }

    static const char*		attribName()	{ return "Hilbert"; }
    static const char*		halflenStr()	{ return "halflen"; }

protected:
    static Provider*		createInstance(Desc&);

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
					    int z0,int nrsamples) const;

    bool			allowParallelComputation() const
    				{ return false; }

//    const Interval<float>*	desZMargin(int input,int output) const;

    const DataHolder*		inputdata_;
    int				dataidx_;

    Interval<float>		gate_;
    Interval<float>		timegate_;
    int				halflen_;
    int				hilbfilterlen_;
    const float*		hilbfilter_;

    static float*		makeHilbFilt(int);
};

}; // namespace Attrib

#endif
