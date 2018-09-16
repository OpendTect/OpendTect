#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bindatadesc.h"
#include "bufstringset.h"
#include "datapack.h"
#include "position.h"
#include "trckeysampling.h"
#include "valseries.h"
#include "arrayndimpl.h"

class FlatPosData;
class Scaler;
class TaskRunner;
namespace ZDomain { class Info; }


/*!\brief DataPack for point data. */

mExpClass(General) PointDataPack : public DataPack
{
public:

    mDeclAbstractMonitorableAssignment(PointDataPack);

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

				PointDataPack(const char*);
				~PointDataPack();

    virtual bool		gtIsEmpty() const	{ return size() < 1; }

};

/*!\brief DataPack for flat data.

  FlatPosData is initialized to ranges of 0 to sz-1 step 1.

  */

mExpClass(General) FlatDataPack : public DataPack
{
public:
				FlatDataPack(const char* categry,
					     Array2D<float>*);
				//!< Array2D become mine (of course)
				mDeclMonitorableAssignment(FlatDataPack);

    virtual Array2D<float>&	data()			{ return *arr2d_; }
    const Array2D<float>&	data() const
				{ return const_cast<FlatDataPack*>(this)
							->data(); }
    virtual float		getPosDistance(bool dim0,float posfidx) const
				{ return mUdf(float); }

    virtual FlatPosData&	posData()		{ return posdata_; }
    const FlatPosData&		posData() const
				{ return const_cast<FlatDataPack*>(this)
							->posData(); }
    virtual const char*		dimName( bool dim0 ) const
				{ return dim0 ? "X1" : "X2"; }

    virtual Coord3		getCoord(int,int) const;
				//!< int,int = Array2D position
				//!< if not overloaded, returns posData() (z=0)

    virtual bool		isVertical() const	{ return true; }
    virtual bool		posDataIsCoord() const	{ return true; }
				// Alternative positions for dim0
    virtual void		getAltDim0Keys(BufferStringSet&) const	{}
				//!< First one is 'default'
    virtual double		getAltDim0Value(int ikey,int idim0) const;
    virtual bool		dimValuesInInt(const char* key) const
				{ return false; }

    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const {}

    virtual int			size(bool dim0) const;

protected:

				FlatDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor
				~FlatDataPack();

    Array2D<float>*		arr2d_;
    FlatPosData&		posdata_;

    virtual bool		gtIsEmpty() const
				{ return size(true)<1 && size(false)<1; }
    virtual float		gtNrKBytes() const;
    virtual void		doDumpInfo(IOPar&) const;
    virtual int			gtNrArrays() const	 { return 1; }
    virtual const ArrayND<float>* gtArrayData(int) const { return arr2d_; }

private:

    void			init();

};


/*!\brief DataPack for 2D data to be plotted on a Map. */

mExpClass(General) MapDataPack : public FlatDataPack
{
public:
				MapDataPack(const char* cat,
					    Array2D<float>*);
				//!<Array becomes mine
				mDeclMonitorableAssignment(MapDataPack);

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

protected:

				~MapDataPack();

    float			getValAtIdx(int,int) const;
    friend class		MapDataPackXYRotater;

    Array2D<float>*		xyrotarr2d_;
    FlatPosData&		xyrotposdata_;
    bool			isposcoord_;
    BufferStringSet		axeslbls_;
    Threads::Lock		initlock_;
};



/*!\brief DataPack for volume data */

mExpClass(General) VolumeDataPack : public DataPack
{
public:

    mDeclAbstractMonitorableAssignment(VolumeDataPack);

    virtual VolumeDataPack*	getSimilar() const			= 0;
    virtual bool		is2D() const				= 0;
    virtual int			nrTrcs() const				= 0;
    virtual TrcKey		getTrcKey(int globaltrcidx) const	= 0;
    virtual int			getGlobalIdx(const TrcKey&) const	= 0;
    virtual int			getNearestGlobalIdx(const TrcKey&) const;
    virtual int			getRandomLineID() const		{ return -1; }

    void			getPath(TrcKeyPath&) const;

    virtual bool		addComponent(const char* nm,bool initvals) = 0;

    virtual const StepInterval<float>&	getZRange() const		= 0;

				//!< Check first if there is a storage!
    const OffsetValueSeries<float> getTrcStorage(
					int comp,int globaltrcidx) const;
    OffsetValueSeries<float>	getTrcStorage(int comp,int globaltrcidx);

				//!< May return null
    const float*		getTrcData(int comp,int globaltrcidx) const;
    float*			getTrcData(int comp,int globaltrcidx);

    bool			getCopiedTrcData(int comp,int globaltrcidx,
						 Array1D<float>&) const;

    int				nrComponents() const
				{ return arrays_.size(); }
    bool			validComp( int comp ) const
				{ return arrays_.validIdx( comp ); }
    void			setComponentName(const char*,int comp=0);
    const char*			getComponentName(int comp=0) const;
    int				getComponentIdx(const char* nm,
						int defcompidx=-1) const;

    static const char*		categoryStr(bool isvertical,bool is2d);

    const Array3DImpl<float>&	data(int component=0) const;
    Array3DImpl<float>&		data(int component=0);

    void			setZDomain(const ZDomain::Info&);
    const ZDomain::Info&	zDomain() const
				{ return *zdomaininfo_; }

    void			setScaler(const Scaler&);
    void			deleteScaler();
    const Scaler*		getScaler() const	{ return scaler_; }

    void			setRefNrs( const TypeSet<float>& refnrs )
				{ refnrs_ = refnrs; }
    float			getRefNr(int globaltrcidx) const;

    const BinDataDesc&		getDataDesc() const	{ return desc_; }
    void			setDataDesc(const BinDataDesc&);
				//<! Will remove incompatible arrays if any

protected:

				VolumeDataPack(const char*,const BinDataDesc*);
				~VolumeDataPack();

    bool			addArray(int sz0,int sz1,int sz2,bool initvals);

    BufferStringSet			componentnames_;
    ObjectSet<Array3DImpl<float> >	arrays_;
    TypeSet<float>			refnrs_;
    ZDomain::Info*			zdomaininfo_;
    BinDataDesc				desc_;
    const Scaler*			scaler_;

    virtual bool		gtIsEmpty() const { return arrays_.isEmpty(); }
    virtual int			gtNrArrays() const { return arrays_.size(); }
    virtual const ArrayND<float>* gtArrayData( int iarr ) const
				{ return arrays_.validIdx(iarr)
				       ? arrays_[iarr] : 0; }

    float			gtNrKBytes() const;
    void			doDumpInfo(IOPar&) const;

};
