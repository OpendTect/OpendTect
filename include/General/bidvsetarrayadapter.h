#ifndef bidvsetarrayadapter_h
#define bidvsetarrayadapter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	H.Huck
 Date:		March 2008
 RCS:		$Id: bidvsetarrayadapter.h,v 1.2 2008-12-25 11:13:33 cvsranojay Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "arrayndinfo.h"
#include "binidvalset.h"


//!\brief an adapter between Array2D and a BinIDValueSet

mClass BIDValSetArrAdapter: 	public Array2D<float>
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
