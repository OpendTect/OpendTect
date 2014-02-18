#ifndef welllogattrib_h
#define welllogattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2013
 RCS:           $Id$
________________________________________________________________________

-*/


#include "wellattribmod.h"
#include "attribprovider.h"
#include "multiid.h"
#include "ranges.h"

template <class T> class Array1D;

namespace Attrib
{

/*!
\brief %WellLog Attribute

Reads a log from 1 well and extends it laterally

<pre>
WellLog

Outputs:
0		Well log data

</pre>
*/

mExpClass(WellAttrib) WellLog : public Provider
{
public:
    static void		initClass();
			WellLog(Desc&);

    static const char*  attribName()		{ return "WellLog"; }
    static const char*	keyStr()		{ return "id"; }
    static const char*	logName()		{ return "logname"; }

    void		prepareForComputeData();

protected:
			~WellLog();
    static Provider*    createInstance(Desc&);

    bool		allowParallelComputation() const;
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    MultiID		wellid_;
    BufferString	logname_;
    Array1D<float>*	logvals_;
    StepInterval<float>	arrzrg_;
};

} // namespace Attrib

#endif

