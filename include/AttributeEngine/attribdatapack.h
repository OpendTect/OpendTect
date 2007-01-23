#ifndef datapackimpl_h
#define datapackimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: attribdatapack.h,v 1.3 2007-01-23 15:30:31 cvshelene Exp $
________________________________________________________________________

-*/

#include "flatdispdatapack.h"
#include "cubesampling.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
namespace Attrib { class DataCubes; }
namespace FlatDisp { class PosData; }


/*!\brief A cube data packet: contains sampled data + positioning */ 

class CubeDataPack : public FlatDispDataPack
{
public:
    				CubeDataPack(Attrib::DataCubes*);
    				~CubeDataPack();

    const CubeSampling		sampling() const;
    const Attrib::DataCubes&	cube() const		{ return *cube_; }
    Attrib::DataCubes&		cube()			{ return *cube_; }

    Array2D<float>&		data();
    const Array2D<float>&	data() const;
    void			positioning(FlatDisp::PosData&);

//    float			nrKBytes() const;
//    void			dumpInfo(IOPar&) const;

protected:

    Attrib::DataCubes*		cube_;
    Array2DSlice<float>*	arr2dsl_;
};


/*!\brief A line data packet: contains sampled data + positioning */

/*
class VertPolyLineDataPack : public DataPack
{
public:

    			VertPolyLineDataPack( Array2D<float>* arr,
					      ObjectSet<Coord3D>* pos )
			    : arr_(arr)
			    , pos_(pos)		{}
			~VertPolyLineDataPack()	{ delete arr_; delete pos_; }

    const ObjectSet<Coord3D>& positions() const	{ return *pos_; }
    ObjectSet<Coord3D>& positions()		{ return *pos_; }
    const Array2D<float>& cube() const		{ return *arr_; }
    Array2D<float>&	cube()			{ return *arr_; }

    float		nrKBytes() const;
    void		dumpInfo(IOPar&) const;

    virtual bool	getInfoAtPos(int,IOPar&) const	{ return false; }

protected:

    CubeSampling	cs_;
    Attrib::DataCube*		cube_;
};
*/

#endif
