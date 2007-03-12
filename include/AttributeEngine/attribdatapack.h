#ifndef attribdatapack_h
#define attribdatapack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: attribdatapack.h,v 1.16 2007-03-12 10:59:35 cvsbert Exp $
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

namespace Attrib
{
class DataCubes;
class Data2DHolder;
class DataHolderArray;

/*!\brief Mixin to provide general services to Attrib data packs */

class DataPackCommon
{
public:
    			DataPackCommon( DescID id )
			    : descid_(id)	{}

    virtual const char*	sourceType() const	= 0;
    virtual bool	isVertical() const	{ return false; }

    DescID		descID() const		{ return descid_; }

    void		dumpInfo(IOPar&) const;

    static const char*	categoryStr(bool vertical);

protected:

    DescID		descid_;

};

/*!\brief Data Pack from 2D attribute data. */

class Flat2DDataPack : public ::FlatDataPack
		     , public DataPackCommon
{
public:
    			Flat2DDataPack(DescID,const Data2DHolder&);
			~Flat2DDataPack();
    virtual const char*	sourceType() const	{ return srctyp_; }
    virtual bool	isVertical() const	{ return true; }

    const Data2DHolder&	dataholder() const	{ return dh_; }
    Array2D<float>&	data();

    Coord3		getCoord(int,int) const;
    void		getAltDim0Keys(BufferStringSet&) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;

    void		dumpInfo(IOPar&) const;
    const char*		dimName(bool) const;

    void		setSourceType( const char* st ) { srctyp_ = st; }
    			//!< Default is "2D", could be set to "Random line"

protected:

    const Data2DHolder& dh_;
    DataHolderArray*	array3d_;
    Array2DSlice<float>* arr2dsl_;
    BufferString	srctyp_;
    TypeSet<SeisTrcInfo::Fld> tiflds_;

    void		setPosData();
};


/*!\brief Flat data pack from 3D attribute extraction */ 

class Flat3DDataPack : public ::FlatDataPack
		     , public DataPackCommon
{
public:

    			Flat3DDataPack(DescID,const DataCubes&,int cubeidx);
    virtual		~Flat3DDataPack();
    virtual const char*	sourceType() const	{ return "3D"; }
    virtual bool	isVertical() const
    			{ return dir_ != CubeSampling::Z; }

    const DataCubes&	cube() const		{ return cube_; }
    Array2D<float>&	data();
    CubeSampling::Dir	dataDir() const		{ return dir_; }
    const char*		dimName(bool) const;

    Coord3		getCoord(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;
    void		dumpInfo(IOPar&) const;

protected:

    const DataCubes&	cube_;
    Array2DSlice<float>* arr2dsl_;
    CubeSampling::Dir	dir_;

    void		setPosData();
};


/*!\brief Volume data pack */ 

class CubeDataPack : public ::CubeDataPack
		   , public DataPackCommon
{
public:

    			CubeDataPack(DescID,const DataCubes&,int cubeidx);
    virtual const char*	sourceType() const	{ return "3D"; }

    const DataCubes&	cube() const		{ return cube_; }
    Array3D<float>&	data();

    void		getAuxInfo(int,int,int,IOPar&) const;
    void		dumpInfo(IOPar&) const;

protected:

    const DataCubes&	cube_;
    int			cubeidx_;

};


} // namespace Attrib

#endif
