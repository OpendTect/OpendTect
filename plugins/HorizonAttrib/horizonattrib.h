#ifndef horizonattrib_h
#define horizonattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: horizonattrib.h,v 1.3 2007-03-08 12:40:08 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "bufstring.h"
#include "multiid.h"

namespace EM { class Horizon; }

namespace Attrib
{

class DataHolder;

class Horizon : public Provider
{
public:
    static void		initClass();
			Horizon(Desc&);
			~Horizon();

    virtual void	prepareForComputeData();

    static const char*	attribName()	{ return "Horizon"; }
    static const char*	sKeyHorID()	{ return "horid"; }
    static const char*	sKeyFileName()	{ return "filename"; }

protected:
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    virtual bool	getInputData(const BinID&,int intv);
    virtual bool	computeData(const DataHolder&,const BinID& relpos,
	    			    int z0,int nrsamples,int threadid) const;

    virtual bool	allowParallelComputation() const { return true; }

    MultiID		horid_;
    BufferString	filenm_;

    EM::Horizon*	horizon_;
    const DataHolder*	inputdata_;
    int			dataidx_;
};

} // namespace Attrib


#endif
