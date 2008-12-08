#ifndef nlacrdesc_h
#define nlacrdesc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2001
 RCS:		$Id: nlacrdesc.h,v 1.9 2008-12-08 12:51:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "nladesign.h"
#include "multiid.h"
#include "bufstringset.h"
#include "iopar.h"
#include "enums.h"
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
				    DataPointSet&) const;

    enum DataType	{ Train=0, Test, MCTrain, MCTest };
    static const char**	DataTypeNames();
    inline static int	dpsGroup( DataType dt )
			{ return ((int)dt) + 1; }
    inline static int	dataTypeOf( int dpsgrp )
			{ return (DataType)(dpsgrp - 1); }
    inline static bool	isTrain( DataType dt )
			{ return dt == Train || dt == MCTrain; }
    inline static bool	isMC( DataType dt )
			{ return dt > Test; }

};


#endif
