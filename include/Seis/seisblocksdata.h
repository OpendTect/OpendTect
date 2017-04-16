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

/*!\brief Storage and access of data in survey-geometry driven blocks.

A 3D survey Geometry defines a full 'Lattice' that in turns defines
a unique set of indices for inlines, crosslines and Z. We can group the
positions into blocks of a size that can easily be read in one go but is still
big enough to not make a huge number of files. BTW the default size 80x80x80
was chosen to always fit within a default HD cache unit of 8MB.

With these predefined dimensions, we can set up indexes for each block in each
dimension (the GlobIdx). Within the blocks, you then have local, relative
indices 0 - N-1 in SampIdx.

  */

namespace Blocks
{
    typedef short		IdxType;
    typedef unsigned short	SzType;
    typedef Survey::Geometry3D	SurvGeom;

#define mDefSeisBlockTripletClass(clss,typ) \
mExpClass(Seis) clss : public Triplet<typ> \
{ \
public: \
\
    inline	clss()					{} \
    inline	clss( short iidx, short xidx, short zidx ) \
		    : Triplet<typ>(iidx,xidx,zidx)	{} \
    inline bool	operator ==( const clss& oth ) const \
		{ return first == oth.first && second == oth.second \
		      && third == oth.third; } \
\
    inline typ	inl() const	{ return first; } \
    inline typ&	inl()		{ return first; } \
    inline typ	crl() const	{ return second; } \
    inline typ&	crl()		{ return second; } \
    inline typ	z() const	{ return third; } \
    inline typ&	z()		{ return third; } \
}

mDefSeisBlockTripletClass(GlobIdx,IdxType);
mDefSeisBlockTripletClass(SampIdx,IdxType);
mDefSeisBlockTripletClass(Dimensions,SzType);


/*!\brief Single block of data */

mExpClass(Seis) Block
{
public:

			Block(GlobIdx,SampIdx start=SampIdx(),
				Dimensions dims=defDims(),
				OD::FPDataRepType fpr=OD::F32);
			~Block();

    const GlobIdx&	globIdx() const		{ return globidx_; }
    const SampIdx&	start() const		{ return start_; }
    const Dimensions&	dims() const		{ return dims_; }
    const DataBuffer&	dataBuf() const		{ return dbuf_; }

    void		zero();
    void		retire();
    bool		isRetired() const;

    float		value(const SampIdx&) const;
    void		getVert(SampIdx,float*,int sz) const;
    void		setValue(const SampIdx&,float);
    void		setVert(SampIdx,const float*,int sz);

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

protected:

    const GlobIdx	globidx_;
    const SampIdx	start_;
    const Dimensions	dims_;
    const Scaler*	scaler_;
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

    virtual		~DataStorage()		{}

    unsigned short	version() const		{ return version_; }

    static BufferString	fileNameFor(const GlobIdx&);

    static const char*	sKeyFileType()		{ return "Seismic Blocks"; }
    static const char*	sKeyDimensions()	{ return "Block Dimensions"; }
    static const char*	sKeyComponents()	{ return "Components"; }
    static const char*	sKeyGlobInlRg()		{ return "Block Inl ID Range"; }
    static const char*	sKeyGlobCrlRg()		{ return "Block Crl ID Range"; }
    static const char*	sKeyGlobZRg()		{ return "Block Z ID Range"; }

protected:

			DataStorage(const SurvGeom*);

    const SurvGeom&	survgeom_;
    const Dimensions	dims_;
    unsigned short	version_;

};


} // namespace Blocks

} // namespace Seis
