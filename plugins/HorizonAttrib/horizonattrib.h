#ifndef horizonattrib_h
#define horizonattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: horizonattrib.h,v 1.1 2006-09-22 09:21:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "bufstring.h"
#include "multiid.h"

namespace Attrib
{

class DataHolder;

class Horizon : public Provider
{
public:
    static void		initClass();
			Horizon( Desc& );

    static const char*	attribName()	{ return "Horizon"; }
    static const char*	sKeyHorID()	{ return "horid"; }
    static const char*	sKeyFileName()	{ return "filename"; }

protected:
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    bool		getInputData(const BinID&,int intv);
    bool		computeData(const DataHolder&,const BinID& relpos,
	    			    int z0,int nrsamples) const;

    bool		allowParallelComputation() const { return true; }

    MultiID		horid_;
    BufferString	filenm_;

    const DataHolder*	inputdata_;
    int			dataidx_;
};

} // namespace Attrib


#endif
