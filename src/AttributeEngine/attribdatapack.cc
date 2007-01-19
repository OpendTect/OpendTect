/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          January 2007
 RCS:           $Id: attribdatapack.cc,v 1.1 2007-01-19 16:18:18 cvshelene Exp $
________________________________________________________________________

-*/

#include "datapackimpl.h"
#include "attribdatacube.h"
#include "flatdisp.h"
#include "arraynd.h"


Array2D<float>& CubeDataPack::data()
{
}


const Array2D<float>& CubeDataPack::data() const
{
    return const_cast<CubeDataPack*>(this)->data();
}


void CubeDataPack::positioning(FlatDisp::PosData&)
{
}

