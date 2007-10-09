#ifndef zaxistransformdatapack_h
#define zaxistransformdatapack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id: zaxistransformdatapack.h,v 1.1 2007-10-09 12:12:31 cvsnanne Exp $
________________________________________________________________________

-*/

#include "datapackbase.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
template <class T> class Array3D;
class FlatPosData;
class ZAxisTransformer;


/*!\brief DataPack for ZAxis transformed data.
*/
    
class ZAxisTransformDataPack : public FlatDataPack
{
public:
    				ZAxisTransformDataPack(const FlatDataPack&,
						       ZAxisTransformer&);
				~ZAxisTransformDataPack();

    bool			transform();

    virtual Array2D<float>&	data();

    virtual void       		dumpInfo(IOPar&) const;

protected:

    const FlatDataPack&		inputdp_;
    ZAxisTransformer&		transformer_;
    const Array3D<float>*	array3d_;
    Array2DSlice<float>*	array2dsl_;
};


#endif
