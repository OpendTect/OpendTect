#ifndef datapackimpl_h
#define datapackimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: attribdatapack.h,v 1.1 2007-01-19 14:50:44 cvshelene Exp $
________________________________________________________________________

-*/

#include "flatdispdatapack.h"
#include "cubesampling.h"

template <class T> class Array2D;
namespace Attrib { class DataCube; }
namespace FlatDisp { class PosData; }


/*!\brief A cube data packet: contains sampled data + positioning */ 

class CubeDataPack : public FlatDispDataPack
{
public:
    				CubeDataPack( Attrib::DataCube* dc,
					      const CubeSampling& cs )
				    : cube_(dc), cs_(cs){}
				~CubeDataPack()		{ delete cube_; }

    const CubeSampling&		sampling() const	{ return cs_; }
    CubeSampling&		sampling()		{ return cs_; }
    const Attrib::DataCube&	cube() const		{ return *cube_; }
    Attrib::DataCube&		cube()			{ return *cube_; }

    Array2D<float>&		data();
    const Array2D<float>&	data() const;
    void			positioning(FlatDisp::PosData&);

    float			nrKBytes() const;
    void			dumpInfo(IOPar&) const;

protected:

    CubeSampling		cs_;
    Attrib::DataCube*		cube_;
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
