#ifndef nlacrdesc_h
#define nlacrdesc_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		June 2001
 RCS:		$Id: nlacrdesc.h,v 1.2 2003-10-17 14:19:01 bert Exp $
________________________________________________________________________

-*/

#include "nladesign.h"
#include "multiid.h"
#include "bufstringset.h"
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

    inline bool		isSupervised() const	{ return design.isSupervised();}

    const char*		transferData(const ObjectSet<FeatureSet>&,
				FeatureSet& train, FeatureSet& test,
				FeatureSet* fswrite=0) const;
			//!< If fswrite not supplied, one will be made
			//!< if necessary.

protected:

    int			addFeatData(FeatureSet&,FeatureSet&,FeatureSet*,
				    const FeatureSet& fs,int,int) const;
};


#endif
