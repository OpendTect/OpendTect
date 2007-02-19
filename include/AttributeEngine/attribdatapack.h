#ifndef attribdatapack_h
#define attribdatapack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: attribdatapack.h,v 1.12 2007-02-19 16:41:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "datapackbase.h"
#include "cubesampling.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
class Coord3;
class IOPar;

namespace Attrib
{
    class DataCubes;
    class Data2DHolder;
    class DataHolderArray;


/*!\brief Data Pack from 2D attribute data. */

class Flat2DDataPack : public ::FlatDataPack
{
public:
    				Flat2DDataPack(const Attrib::Data2DHolder&);
				~Flat2DDataPack();

    const Attrib::Data2DHolder&	dataholder() const	{ return dh_; }
    Array2D<float>&		data();

    void			getAuxInfo(int,int,IOPar&) const;
    Coord3			getCoord(int,int) const;

protected:

    const Attrib::Data2DHolder& dh_;
    Attrib::DataHolderArray*	array3d_;
    Array2DSlice<float>*	arr2dsl_;

    void			setPosData();
};


/*!\brief Flat data pack from 3D attribute extraction */ 

class Flat3DDataPack : public ::FlatDataPack
{
public:
    				Flat3DDataPack(const Attrib::DataCubes&,
					       int cubeidx);
    virtual			~Flat3DDataPack();

    const Attrib::DataCubes&	cube() const		{ return cube_; }
    Array2D<float>&		data();

    void			getAuxInfo(int,int,IOPar&) const;
    Coord3			getCoord(int,int) const;

protected:

    const Attrib::DataCubes&	cube_;
    Array2DSlice<float>*	arr2dsl_;

    void			setPosData();
};


/*!\brief Volume data pack */ 

class CubeDataPack : public ::CubeDataPack
{
public:
    				CubeDataPack(const Attrib::DataCubes&,
					     int cubeidx);

    const Attrib::DataCubes&	cube() const		{ return cube_; }
    Array3D<float>&		data();

    void			getAuxInfo(int,int,int,IOPar&) const;

protected:

    const Attrib::DataCubes&	cube_;
    int				cubeidx_;

};


} // namespace Attrib

#endif
