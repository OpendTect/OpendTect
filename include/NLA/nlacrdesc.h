#ifndef nlacrdesc_h
#define nlacrdesc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2001
 RCS:		$Id: nlacrdesc.h,v 1.8 2008-04-03 11:18:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "nladesign.h"
#include "multiid.h"
#include "bufstringset.h"
#include "iopar.h"
class BinIDValueSet;
class DataPointSet;

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
    MultiID		vdsid;
    float		ratiotst;
    BufferStringSet	outids;
    			//!< different from design outputs if unsupervised
    			//!< Well IDs if direct supervised prediction
    bool		isdirect;
    IOPar		pars;
    			//!< Extra details

    inline bool		isSupervised() const	{ return design.isSupervised();}

    const char*		prepareData(const ObjectSet<DataPointSet>&,
				     DataPointSet& train,
				     DataPointSet& test) const;
};


#endif
