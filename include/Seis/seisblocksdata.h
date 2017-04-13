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
    typedef Survey::Geometry3D	SurvGeom;

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

    GlobIdx		getGlobIdx(const BinID&,const SurvGeom&) const;
    GlobIdx		getGlobIdx(const BinID&,float z,const SurvGeom&) const;
    IdxType		getGlobZIdx(float,const SurvGeom&) const;
    SampIdx		getSampIdx(const BinID&,const SurvGeom&) const;
    SampIdx		getSampIdx(const BinID&,float z,const SurvGeom&) const;
    IdxType		getSampZIdx(float,const SurvGeom&) const;

    static IdxType	globIdx4Inl(const SurvGeom&,int inl,SzType inldim);
    static IdxType	globIdx4Crl(const SurvGeom&,int crl,SzType crldim);
    static IdxType	globIdx4Z(const SurvGeom&,float z,SzType zdim);
    static IdxType	sampIdx4Inl(const SurvGeom&,int inl,SzType inldim);
    static IdxType	sampIdx4Crl(const SurvGeom&,int crl,SzType crldim);
    static IdxType	sampIdx4Z(const SurvGeom&,float z,SzType zdim);
    static int		inl4Idxs(const SurvGeom&,SzType inldim,IdxType globidx,
				IdxType sampidx);
    static int		crl4Idxs(const SurvGeom&,SzType crldim,IdxType globidx,
				IdxType sampidx);
    static float	z4Idxs(const SurvGeom&,SzType zdim,IdxType globidx,
				IdxType sampidx);

    static Dimensions	defDims();

    void		retire();
    bool		isRetired() const;

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

protected:

			DataStorage(const SurvGeom*);

    const SurvGeom&	survgeom_;
    const Dimensions	dims_;

};


} // namespace Blocks

} // namespace Seis
