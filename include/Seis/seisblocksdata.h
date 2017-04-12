#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seiscommon.h"

class Scaler;
class DataBuffer;
namespace Survey { class Geometry3D; }
template <class T> class DataInterpreter;


namespace Seis
{

namespace Blocks
{
    typedef short		IdxType;
    typedef unsigned short	SzType;

#define mDefSeisBlockTripletClass(clss,typ) \
mExpClass(Seis) clss : public Triplet<typ> \
{ \
public: \
		clss()				{} \
		clss( short iidx, short xidx, short zidx ) \
		    : Triplet<typ>(iidx,xidx,zidx)	{} \
 \
    typ		inl() const		{ return first; } \
    typ&	inl()			{ return first; } \
    typ		crl() const		{ return second; } \
    typ&	crl()			{ return second; } \
    typ		z() const		{ return third; } \
    typ&	z()			{ return third; } \
}

mDefSeisBlockTripletClass(GlobIdx,IdxType);
mDefSeisBlockTripletClass(SampIdx,IdxType);
mDefSeisBlockTripletClass(Dimensions,SzType);

/*!\brief Single block of data */

mExpClass(Seis) Data
{
public:

			Data(GlobIdx,Dimensions dims=defDims(),
				OD::FPDataRepType fpr=OD::F32);
			~Data();

    const DataBuffer&	dataBuf() const		{ return dbuf_; }
    float		value(const SampIdx&) const;
    void		getVert(SampIdx,float*,int sz) const;
    void		setValue(const SampIdx&,float);
    void		setVert(SampIdx,const float*,int sz);

    static Dimensions	defDims();

protected:

    const GlobIdx	globidx_;
    const Dimensions	dims_;
    const Scaler*	scaler_;
    const int		totsz_;
    DataBuffer&		dbuf_;
    const DataInterpreter<float>* interp_;

    int			getBufIdx(const SampIdx&) const;
    inline int		nrSampsPerInl() const
			{ return ((int)dims_.crl()) * dims_.z(); }

};


/*!\brief Base class for Reader and Writer */

mExpClass(Seis) DataStorage
{
public:

    virtual		~DataStorage()		    {}

    IdxType		idx4Inl(int) const;
    IdxType		idx4Crl(int) const;
    IdxType		idx4Z(float) const;
    GlobIdx		getGlobIdx(const BinID&,float) const;

protected:

			DataStorage(const Survey::Geometry3D*);

    const Survey::Geometry3D& survgeom_;
    const Dimensions	dims_;

};


} // namespace Blocks

} // namespace Seis
