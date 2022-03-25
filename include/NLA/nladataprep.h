#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2005
________________________________________________________________________

-*/

#include "nlamod.h"
#include "nladesign.h"

class BinIDValueSet;
class PosVecDataSet;
namespace Stats { class RandGen; }

/*!
\brief Prepare data for usage in NLA training.
*/

mExpClass(NLA) NLADataPreparer
{
public:
			NLADataPreparer(BinIDValueSet&,int tc);
			~NLADataPreparer();

    void		removeUndefs(bool targetonly=false);
    void		limitRange(const Interval<float>&);

    struct BalanceSetup
    {
			BalanceSetup()
			: nrclasses(0), nrptsperclss(0), noiselvl(0.01)	{}
	int		nrclasses, nrptsperclss;
	float		noiselvl;
    };
    void		balance(const BalanceSetup&);
			//!< noiselvl not yet supported

protected:

    BinIDValueSet&	bvs_;
    int			targetcol_;
    Stats::RandGen&	gen_;

    void		addVecs(BinIDValueSet&,int,float,
				const Interval<float>*);

};


