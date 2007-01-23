/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          January 2007
 RCS:           $Id: attribdatapack.cc,v 1.2 2007-01-23 15:25:53 cvshelene Exp $
________________________________________________________________________

-*/

#include "datapackimpl.h"
#include "attribdatacubes.h"
#include "flatdisp.h"
#include "arraynd.h"
#include "arrayndslice.h"


CubeDataPack::CubeDataPack( Attrib::DataCubes* dc )
    : cube_(dc)
{
    arr2dsl_ = new Array2DSlice<float>( cube_->getCube(0) ); 
}
					

CubeDataPack::~CubeDataPack()
{
    delete arr2dsl_;
}


Array2D<float>& CubeDataPack::data()
{
    int inlsz = cube_->getInlSz();
    int crlsz = cube_->getInlSz();
    int zsz = cube_->getInlSz();
    int unuseddim = inlsz<2 ? Attrib::DataCubes::cInlDim() 
			    : crlsz<2 ? Attrib::DataCubes::cCrlDim()
			    	      : Attrib::DataCubes::cZDim();
    int dim0 = unuseddim == Attrib::DataCubes::cInlDim() ?
		Attrib::DataCubes::cCrlDim() : Attrib::DataCubes::cInlDim();
    int dim1 = unuseddim == Attrib::DataCubes::cZDim() ?
		Attrib::DataCubes::cCrlDim() : Attrib::DataCubes::cZDim();
    arr2dsl_->setPos( unuseddim, 0 );
    arr2dsl_->setDimMap( 0, dim0 );
    arr2dsl_->setDimMap( 1, dim1 );
    arr2dsl_->init();
    return *arr2dsl_;
}


const Array2D<float>& CubeDataPack::data() const
{
    return const_cast<CubeDataPack*>(this)->data();
}


#define mBuildInterval( rg ) \
    StepInterval<double>( (double)rg.start, (double)rg.stop, (double)rg.step )

void CubeDataPack::positioning( FlatDisp::PosData& posdata )
{
    const CubeSampling cs = sampling();
    int nrinl = cs.nrInl();
    int nrcrl = cs.nrCrl();
    int nrzsamp = cs.nrZ();
    posdata.x1rg_ = nrinl<2 ? mBuildInterval( cs.hrg.crlRange() )
			    : mBuildInterval( cs.hrg.inlRange() );
    posdata.x2rg_ = nrinl>2 ? mBuildInterval( cs.hrg.crlRange() )
			    : mBuildInterval( cs.zrg );
}


const CubeSampling CubeDataPack::sampling() const
{
    return cube_->cubeSampling();
}
