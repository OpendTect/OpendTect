#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2001
________________________________________________________________________

-*/

#include "nlamod.h"
#include "nladesign.h"
#include "multiid.h"
#include "bufstringset.h"
#include "iopar.h"
#include "enums.h"
#include "uistring.h"

class DataPointSet;
namespace Stats { class RandomGenerator; }

/*!
\brief Description of how a NLA analysis Feature set is to be created.
*/

mExpClass(NLA) NLACreationDesc
{ mODTextTranslationClass(NLACreationDesc);
public:
			NLACreationDesc();
			~NLACreationDesc();
			NLACreationDesc(const NLACreationDesc&);
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

    uiString		prepareData(const ObjectSet<DataPointSet>&,
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

private:

    Stats::RandomGenerator& gen_;

};


