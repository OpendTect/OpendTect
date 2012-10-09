#ifndef datapackbase_h
#define datapackbase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "datapack.h"
#include "position.h"
#include "samplingdata.h"
template <class T> class Array2D;
template <class T> class Array3D;

class FlatPosData;
class CubeSampling;
class BufferStringSet;
class TaskRunner;

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
    const Array2D<float>&	rawData() const	{ return *arr2d_; }
    FlatPosData&		posData();
    void			setDimNames(const char*,const char*,bool forxy);
    const char*			dimName( bool dim0 ) const;

    				//!< Alternatively, it can be in Inl/Crl
    bool			posDataIsCoord() const	{ return isposcoord_; }
    void			setPosCoord(bool yn);
    				//!< int,int = Array2D position
    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const;
    void			setProps(StepInterval<double> inlrg,
	    				 StepInterval<double> crlrg,
					 bool,BufferStringSet*);
    void			initXYRotArray(TaskRunner* = 0 );

    void			setRange( StepInterval<double> dim0rg,
	    				  StepInterval<double> dim1rg,
					  bool forxy );

protected:

    float			getValAtIdx(int,int) const;
    friend class 		MapDataPackXYRotater;
    
    Array2D<float>*		xyrotarr2d_;
    FlatPosData&		xyrotposdata_;
    bool			isposcoord_;
    TypeSet<BufferString>	axeslbls_;
    Threads::Mutex		initlock_;
};



/*!\brief DataPack for volume data, where the dims correspond to 
          inl/crl/z . */
    
mClass VolumeDataPack : public DataPack
{
public:

    virtual Array3D<float>&	data();	
    const Array3D<float>&	data() const;

    virtual const char*		dimName(char dim) const;
    virtual double		getPos(char dim,int idx) const;
    int				size(char dim) const;
    virtual float		nrKBytes() const;
    virtual void       		dumpInfo(IOPar&) const;


protected:
    				VolumeDataPack(const char* categry,
					     Array3D<float>*);
				//!< Array3D become mine (of course)
				~VolumeDataPack();

    				VolumeDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    Array3D<float>*		arr3d_;
};

/*!\brief DataPack for volume data, where the dims correspond to 
          inl/crl/z .
*/
    
mClass CubeDataPack : public VolumeDataPack
{
public:
    				CubeDataPack(const char* categry,
					     Array3D<float>*);
				//!< Array2D become mine (of course)
				~CubeDataPack();

    CubeSampling&		sampling()		{ return cs_; }
    const CubeSampling&		sampling() const	{ return cs_; }
    virtual void		getAuxInfo(int,int,int,IOPar&) const {}
    				//!< int,int,int = Array3D position
    Coord3			getCoord(int,int,int) const;
    				//!< int,int,int = Array3D position
    void       			dumpInfo(IOPar&) const;

protected:

    				CubeDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    CubeSampling&		cs_;

private:

    void			init();

};


#endif
