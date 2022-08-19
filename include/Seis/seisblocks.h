#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "filepath.h"
#include "ranges.h"
#include "datachar.h"
#include "zdomain.h"
#include "threadlock.h"
#include "survgeom3d.h"

class DataBuffer;
class LinScaler;
class SeisTrc;
namespace Pos { class IdxPairDataSet; }
namespace PosInfo { class CubeData; }
template <class T> class DataInterpreter;


namespace Seis
{

mGlobal(Seis) inline const char* sSeismicSubDir() { return "Seismics"; }
mGlobal(Seis) inline const char* sInfoFileExtension() { return "info"; }

/*!\brief 3D seismic storage. In 6.2, can be read only.
   For comments see master branch. */

namespace Blocks
{
    typedef short		IdxType;
    typedef unsigned short	SzType;
    typedef StepInterval<float>	ZGeom;
    typedef DataInterpreter<float> DataInterp;
    typedef unsigned short	CoordSysID;

    // From 7.x, this will be a simple typedef from Survey::Geometry3D
    mExpClass(Seis) HGeom : public Survey::Geometry3D
    {
    public:
			HGeom(const Survey::Geometry3D&);
			HGeom(const HGeom&);
	inline int	idx4Inl(int) const;
	inline int	idx4Crl(int) const;
	inline int	idx4Z(float) const;
	inline int	inl4Idx(int) const;
	inline int	crl4Idx(int) const;
	inline float	z4Idx(int) const;
	void		getMapInfo(const IOPar&);
	void		putMapInfo(IOPar&) const;
	bool		isCompatibleWith(const Survey::Geometry&) const;
    };

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
    static int		startInl4GlobIdx(const HGeom&,IdxType,SzType inldim);
    static int		startCrl4GlobIdx(const HGeom&,IdxType,SzType crldim);
    static float	startZ4GlobIdx(const ZGeom&,IdxType,SzType zdim);
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


/*!\brief Base class for Reader and Writer. For comments see master branch. */

mExpClass(Seis) IOClass
{
public:

    typedef DataCharacteristics::UserType	FPDataRepType;

    virtual		~IOClass();

    virtual const HGeom& hGeom() const		{ return hgeom_; }
    const ZGeom&	zGeom() const		{ return zgeom_; }
    const ZDomain::Def&	zDomain() const;

    const Dimensions&	dimensions() const	{ return dims_; }
    SzType		version() const		{ return version_; }
    const char*		cubeName() const	{ return cubename_; }
    const BufferStringSet& componentNames() const { return compnms_; }
    FPDataRepType	fPRep() const		{ return fprep_; }
    const LinScaler*	scaler() const		{ return scaler_; }
    int			nrAuxInfo() const	{ return auxiops_.size(); }
    const IOPar&	getAuxInfo( int i ) const { return *auxiops_[i]; }
    DataType		dataType() const	{ return datatype_; }

    const FilePath&	basePath() const	{ return basepath_; }
    BufferString	infoFileName() const;
    BufferString	dataFileName() const;
    BufferString	overviewFileName() const;
    static BufferString	infoFileNameFor(const char*);
    static BufferString	dataFileNameFor(const char*);

    static const char*	sKeyDataFileExt() { return "blocks"; }
    static const char*	sKeyOvvwFileExt() { return "ovvw"; }
    static const char*	sKeyFileType()	  { return "Column Cube"; }
    static const char*	sKeySectionPre()  { return "Section-"; }
    static const char*	sKeyGenSection()  { return "Section-General"; }
    static const char*	sKeyOffSection()  { return "Section-Offsets"; }
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

protected:

			IOClass();

    mutable Threads::Lock accesslock_;
    Pos::IdxPairDataSet& columns_;

    FilePath		basepath_;
    HGeom&		hgeom_;
    ZGeom		zgeom_;
    Dimensions		dims_;
    SzType		version_;
    BufferString	cubename_;
    BufferStringSet	compnms_;
    LinScaler*		scaler_;
    FPDataRepType	fprep_;
    ObjectSet<IOPar>	auxiops_;
    DataType		datatype_;
    mutable bool	needreset_;

    Column*		findColumn(const HGlobIdx&) const;
    void		addColumn(Column*) const;
    void		clearColumns();
    static SzType	columnHeaderSize(SzType ver);

};


inline int HGeom::idx4Inl( int inl ) const
{ return sampling_.hsamp_.lineIdx( inl ); }
inline int HGeom::idx4Crl( int crl ) const
{ return sampling_.hsamp_.trcIdx( crl ); }
inline int HGeom::idx4Z( float z ) const
{ return sampling_.zsamp_.nearestIndex( z ); }
inline int HGeom::inl4Idx( int idx ) const
{ return sampling_.hsamp_.lineID( idx ); }
inline int HGeom::crl4Idx( int idx ) const
{ return sampling_.hsamp_.traceID( idx ); }
inline float HGeom::z4Idx( int idx ) const
{ return sampling_.zsamp_.atIndex( idx ); }


} // namespace Blocks


} // namespace Seis
