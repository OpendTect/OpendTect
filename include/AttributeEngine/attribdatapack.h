#ifndef attribdatapack_h
#define attribdatapack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: attribdatapack.h,v 1.11 2007-02-02 15:44:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "flatdispdatapack.h"
#include "cubesampling.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
class Coord3;
class IOPar;

namespace FlatDisp { class PosData; }
namespace Attrib
{
    class DataCubes;
    class Data2DHolder;
    class Data2DHolderArray;
}


/*!\brief A cube data packet: contains sampled data + positioning */ 

class CubeDataPack : public FlatDisp::DataPack
{
public:
    				CubeDataPack(const Attrib::DataCubes&);
    				~CubeDataPack();

    const CubeSampling		sampling() const;
    const Attrib::DataCubes&	cube() const		{ return cube_; }

    Array2D<float>&		data();
    const Array2D<float>&	data() const;
    void			positioning(FlatDisp::PosData&);

    float			nrKBytes() const	{ return 0; }
    void			dumpInfo(IOPar&) const	{}
    void			getXYZPosition(PosInfo::Line2DData&) const;

protected:

    const Attrib::DataCubes&	cube_;
    Array2DSlice<float>*	arr2dsl_;
};


/*!\brief A 2D data packet: contains sampled data + positioning */

class DataPack2D : public FlatDisp::DataPack
{
public:
    				DataPack2D(const Attrib::Data2DHolder&);
				~DataPack2D();

    const Attrib::Data2DHolder&	dataholder() const		{ return dh_; }

    Array2D<float>&		data();
    const Array2D<float>&	data() const;
    void			positioning(FlatDisp::PosData&);

    float			nrKBytes() const	{ return 0; }
    void			dumpInfo(IOPar&) const	{}
    virtual bool		getInfoAtPos(int,IOPar&) const
				    { return false; }

protected:
    const Attrib::Data2DHolder& dh_;
    Attrib::Data2DHolderArray*	array3d_;
    Array2DSlice<float>*	arr2dsl_;
};

#endif
