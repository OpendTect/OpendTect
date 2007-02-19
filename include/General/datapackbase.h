#ifndef datapackbase_h
#define datapackbase_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: datapackbase.h,v 1.1 2007-02-19 16:41:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "datapack.h"
#include "position.h"
template <class T> class Array2D;
template <class T> class Array3D;


class FlatPosData;
class CubeSampling;

/*!\brief DataPack for flat data.

  FlatPosData is initialised to ranges of 0 to sz-1 step 1.

  */
    
class FlatDataPack : public DataPack
{
public:
    				FlatDataPack(const char* categry,
					     Array2D<float>*);
				//!< Array2D become mine (of course)
				~FlatDataPack();

    virtual Array2D<float>&	data()			{ return *arr2d_; }
    const Array2D<float>&	data() const
				{ return const_cast<FlatDataPack*>(this)
				    			->data(); }

    FlatPosData&		posData()		{ return posdata_; }
    const FlatPosData&		posData() const		{ return posdata_; }
    virtual void		getAuxInfo(int,int,IOPar&) const {}
    				//!< int,int = Array2D position
    virtual Coord3		getCoord(int,int) const;
    				//!< int,int = Array2D position
    				//!< if not overloaded, returns (posData(),0).
    virtual bool		posDataIsCoord() const	{ return true; }

    virtual float		nrKBytes() const;
    virtual void       		dumpInfo(IOPar&) const;

    virtual int			size(bool dim0) const;

protected:

    				FlatDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    Array2D<float>*		arr2d_;
    FlatPosData&		posdata_;
    const BufferString		category_;

private:

    void			init();

};


/*!\brief DataPack for volume data. */
    
class CubeDataPack : public DataPack
{
public:
    				CubeDataPack(const char* categry,
					     Array3D<float>*);
				//!< Array2D become mine (of course)
				~CubeDataPack();

    virtual Array3D<float>&	data()			{ return *arr3d_; }
    const Array3D<float>&	data() const
				{ return const_cast<CubeDataPack*>(this)
							->data(); }

    CubeSampling&		sampling()		{ return cs_; }
    const CubeSampling&		sampling() const	{ return cs_; }
    virtual void		getAuxInfo(int,int,int,IOPar&) const {}
    				//!< int,int,int = Array3D position
    Coord3			getCoord(int,int,int) const;
    				//!< int,int,int = Array3D position

    virtual float		nrKBytes() const;
    virtual void       		dumpInfo(IOPar&) const;

    virtual int			size(int dim) const;

protected:

    				CubeDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    Array3D<float>*		arr3d_;
    CubeSampling&		cs_;
    const BufferString		category_;

private:

    void			init();

};


#endif
