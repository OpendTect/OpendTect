#ifndef bidvsetarrayadapter_h
#define bidvsetarrayadapter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	H.Huck
 Date:		March 2008
 RCS:		$Id: bidvsetarrayadapter.h,v 1.1 2008-03-21 15:48:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "arrayndinfo.h"
#include "binidvalset.h"


//!\brief an adapter between Array2D and a BinIDValueSet

class BIDValSetArrAdapter: 	public Array2D<float>
{
public:			
    			BIDValSetArrAdapter(const BinIDValueSet&,int);

    void		set(int,int,float);
    float		get(int,int) const;

    const Array2DInfo&	info() const			{ return arrinfo_; }
    Interval<int>	inlrg_;
    Interval<int>	crlrg_;
    

protected:

    Array2DInfoImpl	arrinfo_;
    BinIDValueSet	bidvs_;
    int			targetcolidx_;

};


#endif
