#ifndef eventfreqattrib_h
#define eventfreqattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Jul 2007
 RCS:           $Id: eventfreqattrib.h,v 1.1 2007-07-26 16:35:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "valseries.h"

/*!\brief Computes a simple frequency from distance between events */

namespace Attrib
{

class EventFreq : public Provider
{
public:

    static void		initClass();
			EventFreq(Desc&);

    static const char*	attribName()	   { return "EventFreq"; }

protected:

    static Provider*	createInstance(Desc&);

    bool		allowParallelComputation() const	{ return true; }
    bool		getInputData(const BinID&,int zintv);
    bool		computeData(const DataHolder&,const BinID&,
	    			    int,int,int) const;

    const DataHolder*	inpdata_;
    ValueSeries<float>*	inpseries_;

};

} // namespace Attrib


#endif
