#ifndef attribdatapack_h
#define attribdatapack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "datapackbase.h"
#include "cubesampling.h"
#include "attribdescid.h"
#include "seisinfo.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
class Coord3;
class IOPar;
class SeisTrcBuf;

namespace Attrib
{
class DataCubes;
class Data2DHolder;
class DataHolderArray;
class Data2DArray;

/*!\brief Mixin to provide general services to Attrib data packs */

mClass DataPackCommon
{
public:
    			DataPackCommon( DescID id )
			    : descid_(id)	{}

    virtual const char*	sourceType() const	= 0;
    virtual bool	isVertical() const	{ return false; }

    DescID		descID() const		{ return descid_; }

    void		dumpInfo(IOPar&) const;

    static const char*	categoryStr(bool vertical,bool is2d = false);

protected:

    DescID		descid_;

};


/*!\brief Base class Data Pack for 2D. */

mClass Flat2DDataPack : public ::FlatDataPack
		     , public DataPackCommon
{
public:
    			Flat2DDataPack(DescID);
    virtual const char*	sourceType() const			= 0;
    virtual bool	isVertical() const			{ return true; }

    virtual Array2D<float>&	data()				= 0;

    Coord3		getCoord(int,int) const			= 0;
    void		getAltDim0Keys(BufferStringSet&) const;
    double		getAltDim0Value(int,int) const		= 0;
    void		getAuxInfo(int,int,IOPar&) const	= 0;

    void		dumpInfo(IOPar&) const;
    const char*		dimName(bool) const;

protected:

    TypeSet<SeisTrcInfo::Fld> tiflds_;
};


/*!\brief Data Pack from 2D attribute data. */

mClass Flat2DDHDataPack : public Flat2DDataPack
{
public:
    			Flat2DDHDataPack(DescID,const Data2DHolder&,
					 bool usesingtrc=false,int component=0);
			~Flat2DDHDataPack();
    bool		isOK() const		{ return dataholderarr_; }

    const Data2DArray*	dataarray() const	{ return dataholderarr_; }
    virtual const char*	sourceType() const	{ return "2D"; }

    void		getPosDataTable(TypeSet<int>& trcnrs,
	    				TypeSet<float>& dist) const;
    void		getCoordDataTable(const TypeSet<int>& trcnrs,
	    				  TypeSet<Coord>& coords) const;
    Array2D<float>&	data();

    void		setLineName(const char* nm)	{ linenm_ = nm; }
    void		getLineName(BufferString&) const;
    Coord3		getCoord(int,int) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;

    const CubeSampling&	getCubeSampling() const;

protected:

    Array2DSlice<float>*	array2dslice_;
    const Data2DArray*		dataholderarr_;

    bool			usesingtrc_;
    BufferString		linenm_;

    void			setPosData();
};


/*!\brief Flat data pack from 3D attribute extraction */ 

mClass Flat3DDataPack : public ::FlatDataPack
		     , public DataPackCommon
{
public:

    			Flat3DDataPack(DescID,const DataCubes&,int cubeidx);
    virtual		~Flat3DDataPack();
    virtual const char*	sourceType() const	{ return "3D"; }
    virtual bool	isVertical() const
    			{ return dir_ != CubeSampling::Z; }

    int			getCubeIdx() const	{ return cubeidx_; }
    const DataCubes&	cube() const		{ return cube_; }
    Array2D<float>&	data();
    bool		setDataDir(CubeSampling::Dir);
    CubeSampling::Dir	dataDir() const		{ return dir_; }
    int			nrSlices() const;
    bool		setDataSlice(int);
    int			getDataSlice() const;
    const char*		dimName(bool) const;

    Coord3		getCoord(int,int) const;
    void		getAltDim0Keys(BufferStringSet&) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;
    void		dumpInfo(IOPar&) const;

protected:

    const DataCubes&	cube_;
    Array2DSlice<float>* arr2dsl_;
    Array2D<float>*	arr2dsource_;
    CubeSampling::Dir	dir_;
    bool		usemultcubes_;
    int			cubeidx_;

    void		setPosData();
    void		createA2DSFromMultCubes();
    void		createA2DSFromSingCube(int);
};


/*!\brief Volume data pack */ 

mClass CubeDataPack : public ::CubeDataPack
		   , public DataPackCommon
{
public:

    			CubeDataPack(DescID,const DataCubes&,int cubeidx);
			~CubeDataPack();

    virtual const char*	sourceType() const	{ return "3D"; }

    const DataCubes&	cube() const		{ return cube_; }
    Array3D<float>&	data();

    void		getAuxInfo(int,int,int,IOPar&) const;
    void		dumpInfo(IOPar&) const;

protected:

    const DataCubes&	cube_;
    int			cubeidx_;

};


/*!\brief Data Pack from random traces extraction. */

mClass FlatRdmTrcsDataPack : public Flat2DDataPack
{
public:
    			FlatRdmTrcsDataPack(DescID,const SeisTrcBuf&,
					    TypeSet<BinID>* path=0);
			~FlatRdmTrcsDataPack();
    virtual const char*	sourceType() const	{ return "Random Line"; }

    const SeisTrcBuf&	seisBuf() const		{ return *seisbuf_; }
    Array2D<float>&	data()			{ return *arr2d_; }

    Coord3		getCoord(int,int) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;

    const TypeSet<BinID>* pathBIDs() const { return path_; }

protected:

    SeisTrcBuf* 	seisbuf_;
    Array2D<float>*	arr2d_;

    void		setPosData(TypeSet<BinID>*);
    void		fill2DArray(TypeSet<BinID>*);

    TypeSet<BinID>* path_;
};


} // namespace Attrib

#endif
