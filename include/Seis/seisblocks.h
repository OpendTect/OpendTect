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
#include "ranges.h"
#include "zdomain.h"
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
indices 0 - N-1 in LocIdx.

  */

namespace Blocks
{
    typedef short		IdxType;
    typedef unsigned short	SzType;
    typedef Survey::Geometry3D	HGeom;
    typedef StepInterval<float>	ZGeom;
    typedef DataInterpreter<float> DataInterp;

#define mDefSeisBlockPairClass(clss,typ) \
mExpClass(Seis) clss : public std::pair<typ,typ> \
{ \
public: \
\
    inline	clss() : std::pair<typ,typ>(0,0)		{} \
    inline	clss( typ iidx, typ xidx ) \
		    : std::pair<typ,typ>(iidx,xidx)		{} \
    inline bool	operator ==( const clss& oth ) const \
		{ return first == oth.first && second == oth.second; } \
\
    inline typ	inl() const	{ return first; } \
    inline typ&	inl()		{ return first; } \
    inline typ	crl() const	{ return second; } \
    inline typ&	crl()		{ return second; } \
}

mDefSeisBlockPairClass(HGlobIdx,IdxType);
mDefSeisBlockPairClass(HLocIdx,IdxType);
mDefSeisBlockPairClass(HDimensions,SzType);

#define mDefSeisBlockTripletClass(clss,typ) \
mExpClass(Seis) clss : public H##clss \
{ \
public: \
 \
    inline		clss() : third(0)			{} \
    inline		clss( typ iidx, typ xidx, typ zidx ) \
			    : H##clss(iidx,xidx), third(zidx)	{} \
    inline bool		operator ==( const clss& oth ) const \
			{ return H##clss::operator==(oth) \
			      && third == oth.third; } \
 \
    inline typ		z() const	{ return third; } \
    inline typ&		z()		{ return third; } \
 \
    typ			third; \
 \
}

mDefSeisBlockTripletClass(GlobIdx,IdxType);
mDefSeisBlockTripletClass(LocIdx,IdxType);
mDefSeisBlockTripletClass(Dimensions,SzType);


/*!\brief Base class for single block. */

mExpClass(Seis) Block
{
public:

			~Block()		{}

    const GlobIdx&	globIdx() const		{ return globidx_; }
    const HLocIdx&	start() const		{ return start_; }
    const Dimensions&	dims() const		{ return dims_; }

    static IdxType	globIdx4Inl(const HGeom&,int inl,SzType inldim);
    static IdxType	globIdx4Crl(const HGeom&,int crl,SzType crldim);
    static IdxType	globIdx4Z(const ZGeom&,float z,SzType zdim);
    static IdxType	locIdx4Inl(const HGeom&,int inl,SzType inldim);
    static IdxType	locIdx4Crl(const HGeom&,int crl,SzType crldim);
    static IdxType	locIdx4Z(const ZGeom&,float z,SzType zdim);
    static int		inl4Idxs(const HGeom&,SzType inldim,IdxType globidx,
				IdxType sampidx);
    static int		crl4Idxs(const HGeom&,SzType crldim,IdxType globidx,
				IdxType sampidx);
    static float	z4Idxs(const ZGeom&,SzType zdim,IdxType globidx,
				IdxType loczidx);

    static Dimensions	defDims();

protected:

			Block( const GlobIdx& gidx, const HLocIdx& s,
			       const Dimensions& d )
			    : globidx_(gidx), start_(s), dims_(d)   {}

    const GlobIdx	globidx_;
    const HLocIdx	start_;
    const Dimensions	dims_;

};


/*!\brief Base class for column of blocks. */

mExpClass(Seis) Column
{
public:

    virtual		~Column()					{}

    const HGlobIdx&	globIdx() const		{ return globidx_; }
    const Dimensions&	dims() const		{ return dims_; }
    int			nrComponents() const	{ return nrcomps_; }

protected:

			Column( const HGlobIdx& gidx, const Dimensions& d,
				int nc )
			    : globidx_(gidx), dims_(d), nrcomps_(nc)	{}

    const HGlobIdx	globidx_;
    const Dimensions	dims_;
    const int		nrcomps_;

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
    virtual const HGeom& hGeom() const	= 0;
    const ZGeom&	zGeom() const		{ return zgeom_; }
    const ZDomain::Def&	zDomain() const		{ return zdomaindef_; }

    const Dimensions&	dimensions() const	{ return dims_; }
    SzType		version() const		{ return version_; }
    const char*		cubeName() const	{ return cubename_; }
    const BufferStringSet& componentNames() const { return compnms_; }
    OD::FPDataRepType	fPRep() const		{ return fprep_; }
    const LinScaler*	scaler() const		{ return scaler_; }
    int			nrAuxInfo() const	{ return auxiops_.size(); }
    const IOPar&	getAuxInfo( int i ) const { return *auxiops_[i]; }

    const File::Path&	basePath() const	{ return basepath_; }
    BufferString	infoFileName() const;
    BufferString	dataFileName() const;

    static const char*	sKeyFileType()	 { return "Column Cube"; }
    static const char*	sKeySectionPre() { return "Section-"; }
    static const char*	sKeyGenSection() { return "Section-General"; }
    static const char*	sKeyOffSection() { return "Section-Offsets"; }
    static const char*	sKeyPosSection() { return "Section-Positions"; }
    static const char*	sKeySurveyName() { return "Name.Survey"; }
    static const char*	sKeyCubeName()	 { return "Name.Cube"; }
    static const char*	sKeyFmtVersion() { return "Blocks.Version"; }
    static const char*	sKeyDimensions() { return "Blocks.Max Dimensions"; }
    static const char*	sKeyGlobInlRg()	 { return "Blocks.Inl ID Range"; }
    static const char*	sKeyGlobCrlRg()	 { return "Blocks.Crl ID Range"; }
    static const char*	sKeyGlobZRg()	 { return "Blocks.Z ID Range"; }
    static const char*	sKeyComponents() { return "Components"; }

protected:

			IOClass();

    mutable Threads::Lock accesslock_;
    Pos::IdxPairDataSet& columns_;

    File::Path		basepath_;
    Dimensions		dims_;
    ZGeom		zgeom_;
    ZDomain::Def	zdomaindef_;
    SzType		version_;
    BufferString	cubename_;
    BufferStringSet	compnms_;
    LinScaler*		scaler_;
    OD::FPDataRepType	fprep_;
    ObjectSet<IOPar>	auxiops_;
    mutable bool	needreset_;

    Column*		findColumn(const HGlobIdx&) const;
    void		addColumn(Column*) const;
    void		clearColumns();
    static SzType	columnHeaderSize(SzType ver);

};


} // namespace Blocks

} // namespace Seis
