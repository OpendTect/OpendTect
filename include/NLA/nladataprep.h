#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2005
________________________________________________________________________

-*/

#include "nlamod.h"
#include "nladesign.h"

class BinnedValueSet;
class PosVecDataSet;

/*!
\brief Prepare data for usage in NLA training.
*/

mExpClass(NLA) NLADataPreparer
{
public:
			NLADataPreparer( BinnedValueSet& bvs, int tc )
			    : bvs_(bvs), targetcol_(tc)		{}

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

    BinnedValueSet&	bvs_;
    int			targetcol_;

    void		addVecs(BinnedValueSet&,int,float,
				const Interval<float>*);

};
