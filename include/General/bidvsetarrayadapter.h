#ifndef bidvsetarrayadapter_h
#define bidvsetarrayadapter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H.Huck
 Date:		March 2008
 RCS:		$Id: bidvsetarrayadapter.h,v 1.5 2010/12/02 09:32:42 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "arrayndinfo.h"
#include "binidvalset.h"
#include "horsampling.h"


//!\brief an adapter between Array2D and a BinIDValueSet

mClass BIDValSetArrAdapter : public Array2D<float>
{
public:			
    				BIDValSetArrAdapter(const BinIDValueSet&,
						    int col,const BinID& step);

    void			set(int inlidx,int crlidx,float val);
    float			get(int inlidx,int crlidx) const;

    const Array2DInfo&		info() const		{ return arrinfo_; }
    HorSampling			hrg_;

protected:

    Array2DInfoImpl		arrinfo_;
    const BinIDValueSet&	bidvs_;
    int				targetcolidx_;

};


#endif
