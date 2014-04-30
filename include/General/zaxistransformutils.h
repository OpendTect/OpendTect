#ifndef zaxistransformutils_h
#define zaxistransformutils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "datapackbase.h"
#include "cubesampling.h"
#include "paralleltask.h"
#include "datapackbase.h"
#include "bufstringset.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
template <class T> class Array3D;
class FlatPosData;
class ZAxisTransform;
class DataPointSet;
class BinIDValueSet;

/*!
\brief DataPack for ZAxis transformed data.
*/

mExpClass(General) ZAxisTransformDataPack : public FlatDataPack
{
public:
				ZAxisTransformDataPack(const FlatDataPack&,
					       const CubeSampling& inputcs,
					       ZAxisTransform&);
				~ZAxisTransformDataPack();

    void			setOutputCS(const CubeSampling&);

    bool			transform();

    void			setInterpolate( bool yn ) { interpolate_ = yn; }
    bool			getInterpolate() const	 { return interpolate_;}

    const ZAxisTransform&	getTransform() const	{ return transform_; }
    const CubeSampling& 	inputCS() const 	{ return inputcs_; }

    Array2D<float>&		data();
    const Array2D<float>&	data() const;
    virtual void		dumpInfo(IOPar&) const;

    virtual const char* 	dimName(bool) const;

    static DataPack::ID 	transformDataPack(DataPack::ID,
					const CubeSampling&,ZAxisTransform&);

protected:

    const FlatDataPack& 	inputdp_;
    CubeSampling		inputcs_;
    CubeSampling*		outputcs_;

    ZAxisTransform&		transform_;
    bool			interpolate_;
    int 			voiid_;

    const Array3D<float>*	array3d_;
    Array2DSlice<float>*	array2dsl_;
};


/*!
\brief Generates a DataPointSet with untransformed z-values corresponding to
each BinID and z-value of a specified CubeSampling.
*/

mExpClass(General) ZAxisTransformPointGenerator : public ParallelTask
{
public:
				ZAxisTransformPointGenerator(ZAxisTransform&);
				~ZAxisTransformPointGenerator();

    void			setInput( const CubeSampling& cs );
    void			setOutputDPS( DataPointSet& dps )
				{ dps_ = &dps; }

protected:

    bool			doPrepare(int nrthreads);
    bool			doWork(od_int64,od_int64,int threadid);
    bool			doFinish(bool success);
    od_int64			nrIterations() const
				{ return cs_.hrg.totalNr(); }

    int 			voiid_;
    ObjectSet<BinIDValueSet>	bidvalsets_;
    HorSamplingIterator 	iter_;
    CubeSampling		cs_;
    ZAxisTransform&		transform_;
    DataPointSet*		dps_;
};


/*!
Creates FlatDataPacks from BinIDValueSet. If names are not passed while calling
the function, created datapacks will not have any name. No. of datapacks created
is one less than BinIDValueSet::nrVals(), as the z-component is not used.
*/

mGlobal(General) TypeSet<DataPack::ID> createDataPacksFromBIVSet(
	const BinIDValueSet*,const CubeSampling&,const BufferStringSet& nms=0);


#endif

