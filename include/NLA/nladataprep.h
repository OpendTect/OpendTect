#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    virtual		~NLADataPreparer();

    void		removeUndefs(bool targetonly=false);
    void		limitRange(const Interval<float>&);

    struct BalanceSetup
    {
			BalanceSetup()
			: nrclasses(0), nrptsperclss(0), noiselvl(0.01)
			{}

			~BalanceSetup()
			{}

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
