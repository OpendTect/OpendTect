#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seiscommon.h"
#include "filepath.h"
#include "threadlock.h"

class DataBuffer;
class LinScaler;
class SeisTrc;
namespace Survey { class Geometry3D; }
namespace Pos { class IdxPairDataSet; }
namespace PosInfo { class CubeData; }
template <class T> class DataInterpreter;


namespace Seis
{

/*!\brief Storage and access of data in survey-geometry driven blocks.

A 3D survey Geometry defines a full 'Lattice' that in turns defines
a unique set of indices for inlines, crosslines and Z. We can group the
positions into blocks of a size that is not too big, but big enough to not
make a huge number of files.

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


/*!\brief Base class for single block. */

mExpClass(Seis) Block
{
public:

			~Block();

    const GlobIdx&	globIdx() const		{ return globidx_; }
    const SampIdx&	start() const		{ return start_; }
    const Dimensions&	dims() const		{ return dims_; }

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

    inline static int	nrSampsPerInl( const Dimensions& dims )
			{ return ((int)dims.crl()) * dims.z(); }
    inline static int	nrSampsOnInl( const Dimensions& dims,
				      const SampIdx& sidx )
			{ return ((int)sidx.crl()) * dims.z() + sidx.z(); }

protected:

			Block(GlobIdx,const SampIdx&,const Dimensions&);

    const GlobIdx	globidx_;
    const SampIdx	start_;
    const Dimensions	dims_;
    const DataInterpreter<float>* interp_;

};


/*!\brief Base class for column of blocks. */

mExpClass(Seis) Column
{
public:

    virtual		~Column()		{}

    const GlobIdx	globidx_;
    const Dimensions	dims_;
    const int		nrcomps_;

protected:

			Column(const GlobIdx&,const Dimensions&,int);

};


/*!\brief Base class for Reader and Writer.

  The format is designed with these principles in mind:
  * Bricking helps keep performance similar and reasonably OK in all directions
  * The total geometry setup is stored in a human-readbale summary file, which
    will make the data usable accross surveys
  * When writing, no sorting is required, although some sorting will help keep
    memory consumption down

*/

mExpClass(Seis) IOClass
{
public:

    virtual		~IOClass();
    virtual const SurvGeom& survGeom() const	= 0;

    const Dimensions&	dimensions() const	{ return dims_; }
    unsigned short	version() const		{ return version_; }
    const File::Path&	basePath() const	{ return basepath_; }
    const char*		fileNameBase() const	{ return filenamebase_; }
    const char*		cubeName(const char*) const { return cubename_; }
    const BufferStringSet& componentNames() const { return compnms_; }
    OD::FPDataRepType	fPRep() const		{ return fprep_; }
    const LinScaler*	scaler() const		{ return scaler_; }
    int			nrAuxInfo() const	{ return auxiops_.size(); }
    const IOPar&	getAuxInfo( int i ) const { return *auxiops_[i]; }

    BufferString	dataDirName() const;
    BufferString	mainFileName() const;

    static BufferString	fileNameFor(const GlobIdx&);

    static const char*	sKeyFileType()	 { return "Column Cube"; }
    static const char*	sKeySectionPre() { return "Section-"; }
    static const char*	sKeyGenSection() { return "Section-General"; }
    static const char*	sKeyPosSection() { return "Section-Positions"; }
    static const char*	sKeySurveyName() { return "Name.Survey"; }
    static const char*	sKeyCubeName()	 { return "Name.Cube"; }
    static const char*	sKeyFmtVersion() { return "Blocks.Version"; }
    static const char*	sKeyDimensions() { return "Blocks.Max Dimensions"; }
    static const char*	sKeyGlobInlRg()	 { return "Blocks.Inl ID Range"; }
    static const char*	sKeyGlobCrlRg()	 { return "Blocks.Crl ID Range"; }
    static const char*	sKeyGlobZRg()	 { return "Blocks.Z ID Range"; }
    static const char*	sKeyComponents() { return "Components"; }

    typedef unsigned short  HdrSzVersionType;

protected:

			IOClass();

    mutable Threads::Lock accesslock_;
    Pos::IdxPairDataSet& columns_;

    File::Path		basepath_;
    Dimensions		dims_;
    HdrSzVersionType	version_;
    BufferString	filenamebase_;
    BufferString	cubename_;
    BufferStringSet	compnms_;
    LinScaler*		scaler_;
    OD::FPDataRepType	fprep_;
    ObjectSet<IOPar>	auxiops_;
    mutable bool	needreset_;

    Column*		findColumn(const GlobIdx&) const;
    void		addColumn(Column*) const;
    void		clearColumns();
    static HdrSzVersionType columnHeaderSize(HdrSzVersionType);

};


} // namespace Blocks

} // namespace Seis
