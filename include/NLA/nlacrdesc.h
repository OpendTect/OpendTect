#ifndef nlacrdesc_h
#define nlacrdesc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2001
 RCS:		$Id: nlacrdesc.h,v 1.5 2005-01-31 16:03:19 bert Exp $
________________________________________________________________________

-*/

#include "nladesign.h"
#include "multiid.h"
#include "bufstringset.h"
#include "iopar.h"
class FeatureSet;

/*\brief Description of how an NLA analysis Feature set is to be created */

class NLACreationDesc
{
public:
    			NLACreationDesc()	{ clear(); }
			~NLACreationDesc()	{ clear(); }
			NLACreationDesc( const NLACreationDesc& sd )
						{ *this = sd; }
    NLACreationDesc& operator =(const NLACreationDesc&);
    void		clear();

    NLADesign		design;
    bool		doextraction;
    MultiID		fsid;
    float		ratiotst;
    BufferStringSet	outids;
    			//!< different from design outputs if unsupervised
    			//!< Well IDs if direct supervised prediction
    bool		isdirect;
    IOPar		pars;
    			//!< Extra details

    inline bool		isSupervised() const	{ return design.isSupervised();}

    const char*		transferData(const ObjectSet<FeatureSet>&,
				FeatureSet& train, FeatureSet& test) const;

protected:

    int			addFeatData(FeatureSet&,FeatureSet&,const FeatureSet&,
	    			    int,int) const;
};


#endif
