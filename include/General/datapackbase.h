#ifndef datapackbase_h
#define datapackbase_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: datapackbase.h,v 1.12 2008-12-25 11:21:53 cvsranojay Exp $
________________________________________________________________________

-*/

#include "datapack.h"
#include "position.h"
template <class T> class Array2D;
template <class T> class Array3D;

class FlatPosData;
class CubeSampling;
class BufferStringSet;

/*!\brief DataPack for point data. */
    
mClass PointDataPack : public DataPack
{
public:

    virtual int			size() const			= 0;
    virtual BinID		binID(int) const		= 0;
    virtual float		z(int) const			= 0;
    virtual Coord		coord(int) const;
    virtual int			trcNr(int) const		{ return 0; }

    virtual bool		simpleCoords() const		{ return true; }
    				//!< If true, coords are always SI().tranform(b)
    virtual bool		isOrdered() const		{ return false;}
    				//!< If yes, one can draw a line between the pts

protected:

    				PointDataPack( const char* categry )
				    : DataPack( categry )	{}

};

/*!\brief DataPack for flat data.

  FlatPosData is initialised to ranges of 0 to sz-1 step 1.

  */
    
mClass FlatDataPack : public DataPack
{
public:
    				FlatDataPack(const char* categry,
					     Array2D<float>*);
				//!< Array2D become mine (of course)
				FlatDataPack(const FlatDataPack&);
				~FlatDataPack();

    virtual Array2D<float>&	data()			{ return *arr2d_; }
    const Array2D<float>&	data() const
				{ return const_cast<FlatDataPack*>(this)
				    			->data(); }

    virtual FlatPosData&	posData()		{ return posdata_; }
    const FlatPosData&		posData() const
				{ return const_cast<FlatDataPack*>(this)
				    			->posData(); }
    virtual const char*		dimName( bool dim0 ) const
				{ return dim0 ? "X1" : "X2"; }

    virtual Coord3		getCoord(int,int) const;
    				//!< int,int = Array2D position
    				//!< if not overloaded, returns posData() (z=0)

    virtual bool		posDataIsCoord() const	{ return true; }
				// Alternative positions for dim0
    virtual void		getAltDim0Keys(BufferStringSet&) const {}
    				//!< First one is 'default'
    virtual double		getAltDim0Value(int ikey,int idim0) const;

    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const {}

    virtual float		nrKBytes() const;
    virtual void       		dumpInfo(IOPar&) const;

    virtual int			size(bool dim0) const;

protected:

    				FlatDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    Array2D<float>*		arr2d_;
    FlatPosData&		posdata_;

private:

    void			init();

};


/*!\brief DataPack for 2D data to be plotted on a Map. */

mClass MapDataPack : public FlatDataPack
{
public:
    				MapDataPack(const char* categry,const char* nm,
					    Array2D<float>*);
				~MapDataPack();

    Array2D<float>&		data();
    FlatPosData&		posData();
    void			setDimNames(const char*,const char*,bool set1);
    const char*			dimName( bool dim0 ) const;

    				//!< Alternatively, it can be in Inl/Crl
    bool			posDataIsCoord() const	{ return isposcoord_; }
    void			setPosCoord(bool yn)	{ isposcoord_ = yn; }
    Coord			get2DCoord(int,int) const;
    				//!< int,int = Array2D position
    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const;
    void			setPropsAndInit(StepInterval<double> dim0rg,
	    					StepInterval<double> dim1rg,
						bool,BufferStringSet*);

protected:

    void			createXYRotArray();
    float			getValAtIdx(int,int) const;
    
    Array2D<float>*		xyrotarr2d_;
    FlatPosData&		fakeposdata_;
    bool			isposcoord_;
    bool			mainisset1_; //for maps X/Y is the default set1
    TypeSet<BufferString>	axeslbls_;
};

/*!\brief DataPack for volume data. */
    
mClass CubeDataPack : public DataPack
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

private:

    void			init();

};


#endif
