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
#include "threadlock.h"
#include "iopar.h"
#include "zdomain.h"

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
positions into blocks of a size that makes it reasonable for efficient storage.

With these predefined dimensions, we can set up indexes for each block in each
dimension (the GlobIdx). Within the blocks, you then have local, relative
indices 0 - N-1 (the LocIdx).

  */

namespace Blocks
{
    typedef od_uint16		version_type;
    typedef od_uint16		size_type;
    typedef od_int16		idx_type;
    typedef Survey::Geometry3D	HGeom;
    typedef StepInterval<float>	ZGeom;
    typedef DataInterpreter<float> DataInterp;

#define mDefSeisBlocksPairClass(clss,typ) \
mExpClass(Seis) clss : public Twins<typ> \
{ \
public: \
\
    inline	clss()	: Twins<typ>(0,0)		{} \
    inline	clss( typ iidx, typ xidx ) \
			: Twins<typ>(iidx,xidx)		{} \
    inline bool	operator ==( const clss& oth ) const \
		{ return first == oth.first && second == oth.second; } \
\
    inline typ	inl() const	{ return first; } \
    inline typ&	inl()		{ return first; } \
    inline typ	crl() const	{ return second; } \
    inline typ&	crl()		{ return second; } \
}

mDefSeisBlocksPairClass(HGlobIdx,idx_type);
mDefSeisBlocksPairClass(HLocIdx,idx_type);
mDefSeisBlocksPairClass(HDimensions,size_type);

#define mDefSeisBlocksTripletClass(clss,typ) \
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
    inline clss&	set( const H##clss& oth ) \
			{ first=oth.first; second=oth.second; return *this; } \
    inline clss&	set( const clss& oth ) \
			{ *this = oth; return *this; } \
 \
    inline typ		z() const	{ return third; } \
    inline typ&		z()		{ return third; } \
 \
    typ			third; \
 \
}

mDefSeisBlocksTripletClass(GlobIdx,idx_type);
mDefSeisBlocksTripletClass(LocIdx,idx_type);
mDefSeisBlocksTripletClass(Dimensions,size_type);


/*!\brief Base class for single block. */

mExpClass(Seis) Block
{
public:

			~Block()		{}

    const GlobIdx&	globIdx() const		{ return globidx_; }
    const HLocIdx&	start() const		{ return start_; }
    const Dimensions&	dims() const		{ return dims_; }

    static idx_type	globIdx4Inl(const HGeom&,int inl,size_type inlsz);
    static idx_type	globIdx4Crl(const HGeom&,int crl,size_type crlsz);
    static idx_type	globIdx4Z(const ZGeom&,float z,size_type zsz);
    static idx_type	locIdx4Inl(const HGeom&,int inl,size_type inlsz);
    static idx_type	locIdx4Crl(const HGeom&,int crl,size_type crlsz);
    static idx_type	locIdx4Z(const ZGeom&,float z,size_type zdim);
    static int		startInl4GlobIdx(const HGeom&,idx_type,size_type inlsz);
    static int		startCrl4GlobIdx(const HGeom&,idx_type,size_type crlsz);
    static float	startZ4GlobIdx(const ZGeom&,idx_type,size_type zsz);
    static int		inl4Idxs(const HGeom&,size_type inlsz,idx_type globidx,
				idx_type sampidx);
    static int		crl4Idxs(const HGeom&,size_type crlsz,idx_type globidx,
				idx_type sampidx);
    static float	z4Idxs(const ZGeom&,size_type zsz,idx_type globidx,
				idx_type loczidx);

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

mExpClass(Seis) Access
{
public:

    virtual		~Access();

    virtual const HGeom& hGeom() const		{ return hgeom_; }
    const ZGeom&	zGeom() const		{ return zgeom_; }

    const Dimensions&	dimensions() const	{ return dims_; }
    version_type	version() const		{ return version_; }
    const char*		cubeName() const	{ return cubename_; }
    const BufferStringSet& componentNames() const { return compnms_; }
    OD::DataRepType	dataRep() const		{ return datarep_; }
    const LinScaler*	scaler() const		{ return scaler_; }
    int			nrAuxInfo() const	{ return auxiops_.size(); }
    const IOPar&	getAuxInfo( int i ) const { return *auxiops_[i]; }
    DataType		dataType() const	{ return datatype_; }
    const ZDomain::Def&	zDomain() const		{ return zdomain_; }

    const File::Path&	basePath() const	{ return basepath_; }
    BufferString	infoFileName() const;
    const char*		fileExtension() const;
    BufferString	dataFileName() const;
    BufferString	overviewFileName() const;
    static BufferString	infoFileNameFor(const char*);
    static BufferString	dataFileNameFor(const char*,bool usehdf);

    static const char*	sDataFileExt(bool forhdf5);
    static const char*	sKeyOvvwFileExt() { return "ovvw"; }
    static const char*	sKeyFileType()	  { return "Column Cube"; }
    static const char*	sKeySectionPre()  { return "Section-"; }
    static const char*	sKeyGenSection()  { return "Section-General"; }
    static const char*	sKeyOffSection()  { return "Section-Offsets"; }
    static const char*	sKeyFileIDSection()  { return "Section-FileIDs"; }
    static const char*	sKeyPosSection()  { return "Section-Positions"; }
    static const char*	sKeySurveyName()  { return "Name.Survey"; }
    static const char*	sKeyCubeName()	  { return "Name.Cube"; }
    static const char*	sKeyFmtVersion()  { return "Blocks.Version"; }
    static const char*	sKeyDimensions()  { return "Blocks.Max Dimensions"; }
    static const char*	sKeyGlobInlRg()	  { return "Blocks.Inl ID Range"; }
    static const char*	sKeyGlobCrlRg()	  { return "Blocks.Crl ID Range"; }
    static const char*	sKeyGlobZRg()	  { return "Blocks.Z ID Range"; }
    static const char*	sKeyComponents()  { return "Components"; }
    static const char*	sKeyDataType()    { return "Data Type"; }
    static const char*	sKeyDepthInFeet() { return "Depth in Feet"; }

    static bool		hdf5Active();

protected:

			Access();

    mutable Threads::Lock accesslock_;
    Pos::IdxPairDataSet& columns_;

    File::Path		basepath_;
    HGeom&		hgeom_;
    ZGeom		zgeom_;
    ZDomain::Def	zdomain_;
    Dimensions		dims_;
    version_type	version_;
    BufferString	cubename_;
    BufferStringSet	compnms_;
    LinScaler*		scaler_;
    OD::DataRepType	datarep_;
    IOPar		gensectioniop_;
    IOPar		fileidsectioniop_;
    ObjectSet<IOPar>	auxiops_;
    PosInfo::CubeData&	cubedata_;
    DataType		datatype_;
    mutable bool	needreset_;
    mutable bool	usehdf_;

    Column*		findColumn(const HGlobIdx&) const;
    void		addColumn(Column*) const;
    void		clearColumns();

};


} // namespace Blocks

} // namespace Seis
