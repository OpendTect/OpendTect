#ifndef hilbertattrib_h
#define hilbertattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: hilbertattrib.h,v 1.2 2005-06-02 10:37:53 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "runstat.h"
#include "valseries.h"
#include "valseriesinterpol.h"
#include "mathfunc.h"

namespace Attrib
{


class Hilbert : public Provider
{
public:
    static void			initClass();
				Hilbert(Desc&);

    static const char*		attribName()	{ return "Hilbert"; }
    static const char*		halflenStr()	{ return "halflen"; }

protected:
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);
    static Provider*		internalCreate(Desc&,ObjectSet<Provider>& exis);

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
					    int t0,int nrsamples) const;

    const Interval<float>*	reqZMargin(int input,int output) const;

    const DataHolder*		inputdata;

    Interval<float>		gate;
    int				halflen;
    int				hilbfilterlen;
    const float*		hilbfilter;
    static float*		makeHilbFilt(int);
};

}; // namespace Attrib


#endif

