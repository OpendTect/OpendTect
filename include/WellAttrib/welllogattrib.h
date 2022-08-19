#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "attribprovider.h"
#include "multiid.h"
#include "ranges.h"
#include "stattype.h"

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
{ mODTextTranslationClass(WellLog)
public:
    static void		initClass();
			WellLog(Desc&);

    static const char*  attribName()		{ return "WellLog"; }
    static const char*	keyStr()		{ return "id"; }
    static const char*	logName()		{ return "logname"; }
    static const char*	upscaleType()		{ return "upscaletype"; }

    void		prepareForComputeData() override;

protected:
			~WellLog();
    static Provider*    createInstance(Desc&);

    bool		allowParallelComputation() const override;
    bool		getInputOutput(int input,
				       TypeSet<int>& res) const override;
    bool		getInputData(const BinID&, int idx) override;
    bool		computeData(const DataHolder&,const BinID& relpos,
			    int t0,int nrsamples,int threadid) const override;

    MultiID		wellid_;
    BufferString	logname_;
    int			upscaletype_; // Stats::UpscaleType
    Array1D<float>*	logvals_;
    StepInterval<float>	arrzrg_;
};

} // namespace Attrib
