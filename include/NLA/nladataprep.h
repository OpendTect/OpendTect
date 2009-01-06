#ifndef nladataprep_h
#define nladataprep_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2005
 RCS:		$Id: nladataprep.h,v 1.4 2009-01-06 05:22:16 cvsranojay Exp $
________________________________________________________________________

-*/

#include "nladesign.h"
class BinIDValueSet;
class PosVecDataSet;
template <class T> class Interval;

/*\brief Prepare data for usage in NLA training */

mClass NLADataPreparer
{
public:
    			NLADataPreparer( BinIDValueSet& bvs, int tc )
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

    BinIDValueSet&	bvs_;
    int			targetcol_;

    void		addVecs(BinIDValueSet&,int,float,
	    			const Interval<float>*);
    void		removeVecs(BinIDValueSet&,int);

};


#endif
