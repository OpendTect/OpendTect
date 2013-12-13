#ifndef welllogattrib_h
#define welllogattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/


#include "wellattribmod.h"
#include "attribprovider.h"


namespace Attrib
{

/*!
\brief %WellLog Attribute

Reads a log from 1 or more wells

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

protected:
			~WellLog() {}
    static Provider*    createInstance(Desc&);

    bool		allowParallelComputation() const;
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    Interval<float>	gate_;
    Interval<int>	dessampgate_;
    int			dataidx_;
    const DataHolder*	inputdata_;
};

} // namespace Attrib


#endif

