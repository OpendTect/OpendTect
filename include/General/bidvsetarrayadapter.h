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
#include "binnedvalueset.h"
#include "trckeysampling.h"


//!\brief an adapter offering the Array2D interface for a BinnedValueSet

mExpClass(General) BIDValSetArrAdapter : public Array2D<float>
{
public:
				BIDValSetArrAdapter(const BinnedValueSet&,
						    int col,const BinID& step);

    void			set(int inlidx,int crlidx,float val);
    float			get(int inlidx,int crlidx) const;

    const Array2DInfo&		info() const		{ return arrinfo_; }

    TrcKeySampling		tks_;

protected:

    Array2DInfoImpl		arrinfo_;
    const BinnedValueSet&	bidvs_;
    int				targetcolidx_;

};
