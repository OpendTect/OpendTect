#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H.Huck
 Date:		March 2008
________________________________________________________________________

-*/

#include "generalmod.h"
#include "arraynd.h"
#include "arrayndinfo.h"
#include "binidvalset.h"
#include "trckeysampling.h"


//!\brief an adapter between Array2D and a BinIDValueSet

mExpClass(General) BIDValSetArrAdapter : public Array2D<float>
{
public:			
				BIDValSetArrAdapter(const BinIDValueSet&,
						    int col,const BinID& step);

    void			set(int inlidx,int crlidx,float val) override;
    float			get(int inlidx,int crlidx) const override;

    const Array2DInfo&		info() const override	{ return arrinfo_; }
    TrcKeySampling			tks_;

protected:

    Array2DInfoImpl		arrinfo_;
    const BinIDValueSet&	bidvs_;
    int				targetcolidx_;

};


