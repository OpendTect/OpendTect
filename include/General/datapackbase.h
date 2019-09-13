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
#include "valseries.h"
#include "arrayndimpl.h"

class BinnedValueSet;
class FlatPosData;
class Scaler;
class TrcKey;
class TrcKeyPath;
namespace ZDomain   { class Info; }
namespace Pos	    { class IdxSubSel2D; }


/*!\brief DataPack for point data. */

mExpClass(General) PointDataPack : public DataPack
{
public:

    typedef Pos::Z_Type		z_type;
    typedef Pos::TraceNr_Type	trcnr_type;
    typedef trcnr_type		linenr_type;
    mUseType( Pos,		GeomID );

    mDeclAbstractMonitorableAssignment(PointDataPack);

    virtual bool	is2D() const			= 0;
    virtual size_type	size() const			= 0;
    virtual z_type	z(idx_type) const		= 0;
    virtual linenr_type	lineNr(idx_type) const		= 0;
    virtual trcnr_type	trcNr(idx_type) const		= 0;
    virtual Coord	coord(idx_type) const;
    virtual BinID	binID(idx_type) const;
    virtual Bin2D	bin2D(idx_type) const;

    virtual bool	simpleCoords() const		{ return true; }
				//!< If true, coords are always SI().tranform(b)
    virtual bool	isOrdered() const		{ return false;}
				//!< If yes, one can draw a line between the pts

    GeomID		geomID(idx_type) const;

protected:

			PointDataPack(const char*);
			~PointDataPack();

    virtual bool	gtIsEmpty() const override	{ return size() < 1; }

};

/*!\brief DataPack for flat data.

  FlatPosData is initialized to ranges of 0 to sz-1 step 1.

  */

mExpClass(General) FlatDataPack : public DataPack
{
public:

    typedef Array2D<value_type>		data_type;
    typedef Array2DImpl<value_type>	impl_type;

				FlatDataPack(const char* categry,data_type*);
					//!< arr becomes mine (of course)
				mDeclMonitorableAssignment(FlatDataPack);

    virtual data_type&	data()	{ return *arr2d_; }
    const data_type&	data() const
				{ return const_cast<FlatDataPack*>(this)
							->data(); }
    virtual float	getPosDistance(bool dim0,float posfidx) const
				{ return mUdf(float); }

    virtual FlatPosData& posData()
				{ return posdata_; }
    const FlatPosData&	posData() const
				{ return mSelf().posData(); }
    virtual const char*	dimName( bool dim0 ) const
				{ return dim0 ? "X1" : "X2"; }

    Coord3		getCoord(idx_type,idx_type) const;
				//!< if not overloaded, returns posData() (z=0)

    virtual bool	isVertical() const	{ return true; }
    virtual bool	posDataIsCoord() const	{ return true; }

    virtual void	getAltDim0Keys(BufferStringSet&) const	{}
				//!< First one is 'default'
    virtual double	getAltDim0Value(idx_type ikey,idx_type idim0) const;
    virtual bool	dimValuesInInt( const char* key ) const
				{ return false; }

    virtual void	getAuxInfo(idx_type idim0,idx_type idim1,IOPar&) const
			{}

    virtual size_type	size(bool dim0) const;

protected:

			FlatDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor
			~FlatDataPack();

    data_type*		arr2d_;
    FlatPosData&	posdata_;

    virtual bool	gtIsEmpty() const override
				{ return size(true)<1 || size(false)<1; }
    virtual float	gtNrKBytes() const override;
    virtual void	doDumpInfo(IOPar&) const override;
    virtual size_type	gtNrArrays() const override	{ return 1; }
    virtual const arrnd_type* gtArrayData( idx_type ) const override
							{ return arr2d_; }

private:

    void		init();

};


/*!\brief DataPack for 2D data to be plotted on a Map. The array indices then
 correspond to inlines and crosslines. */

mExpClass(General) MapDataPack : public FlatDataPack
{
public:

    mUseType( Pos,	IdxSubSel2D );

			MapDataPack(const char* cat,data_type*);
					//!< arr becomes mine (of course)
			mDeclMonitorableAssignment(MapDataPack);

    virtual bool	isVertical() const override	{ return false; }

    void		setPositions(const IdxSubSel2D&);
    const IdxSubSel2D&	positions() const	{ return *idxsubsel_; }
    void		setDimNames(const char*,const char*);
				// defaults to inline and crossline
    const char*		dimName(bool dim0) const override;

    virtual void	getAuxInfo(idx_type,idx_type,IOPar&) const override;

protected:

			~MapDataPack();

    IdxSubSel2D*	idxsubsel_;
    BufferString	dim0nm_;
    BufferString	dim1nm_;

};



/*!\brief DataPack for volume data */

mExpClass(General) VolumeDataPack : public DataPack
{
public:

    typedef Pos::Z_Type			z_type;
    typedef Interval<z_type>		z_rg_type;
    typedef StepInterval<z_type>	z_steprg_type;
    typedef Array3D<value_type>		data_type;
    typedef Array3DImpl<value_type>	impl_type;
    typedef idx_type			glob_idx_type;
    typedef size_type			glob_size_type;
    typedef idx_type			comp_idx_type;
    typedef int				rdl_id;

    mDeclAbstractMonitorableAssignment(VolumeDataPack);

    virtual bool		is2D() const				= 0;
    virtual glob_size_type	nrPositions() const			= 0;
    virtual VolumeDataPack*	getSimilar() const			= 0;
    virtual z_steprg_type	zRange() const				= 0;
    virtual rdl_id		randomLineID() const		{ return -1; }

    virtual void		getPath(TrcKeyPath&) const;
    virtual void		getPositions(BinnedValueSet&) const;

    virtual bool		addComponent(const char* nm,bool initvals) = 0;

				//!< Check first if there is a storage!
    OffsetValueSeries<float>	getTrcStorage(
					comp_idx_type,glob_idx_type) const;

				//!< May return null
    const float*		getTrcData(comp_idx_type,glob_idx_type) const;
    float*			getTrcData(comp_idx_type,glob_idx_type);

    bool			getCopiedTrcData(comp_idx_type,glob_idx_type,
						 Array1D<float>&) const;

    comp_idx_type		nrComponents() const
					{ return arrays_.size(); }
    bool			validComp( comp_idx_type comp ) const
					{ return arrays_.validIdx( comp ); }
    void			setComponentName(const char*,comp_idx_type c=0);
    const char*			getComponentName(comp_idx_type c=0) const;
    comp_idx_type				getComponentIdx(const char* nm,
						comp_idx_type defidx=-1) const;

    static const char*		categoryStr(bool isvertical,bool is2d);

    impl_type&			data( comp_idx_type c=0 )
				{ return *arrays_[c]; }
    const impl_type&		data( comp_idx_type c=0 ) const
				{ return *arrays_[c]; }

    BinID			binID(glob_idx_type) const;
    Bin2D			bin2D(glob_idx_type) const;
    glob_idx_type		globalIdx(const BinID&) const;
    glob_idx_type		globalIdx(const Bin2D&) const;
    glob_idx_type		nearestGlobalIdx(const BinID&) const;
    glob_idx_type		nearestGlobalIdx(const Bin2D&) const;
    void			getTrcKey( glob_idx_type i, TrcKey& tk ) const
				{ gtTrcKey( i, tk ); }
    glob_idx_type		getGlobalIdx( const TrcKey& tk ) const
				{ return gtGlobalIdx( tk ); }
    glob_idx_type		getNearestGlobalIdx(const TrcKey&) const;

    void			setZDomain(const ZDomain::Info&);
    const ZDomain::Info&	zDomain() const
				{ return *zdomaininfo_; }

    void			setScaler(const Scaler&);
    void			deleteScaler();
    const Scaler*		getScaler() const	{ return scaler_; }

    void			setRefNrs( const TypeSet<float>& refnrs )
				{ refnrs_ = refnrs; }
    float			getRefNr(glob_idx_type) const;

    const BinDataDesc&		getDataDesc() const	{ return desc_; }
    void			setDataDesc(const BinDataDesc&);
				//<! Will remove incompatible arrays if any

protected:

				VolumeDataPack(const char*,const BinDataDesc*);
				~VolumeDataPack();

    bool			addArray(size_type,size_type,size_type,
					 bool initvals);

    BufferStringSet		componentnames_;
    ObjectSet<impl_type>	arrays_;
    TypeSet<float>		refnrs_;
    ZDomain::Info*		zdomaininfo_;
    BinDataDesc			desc_;
    const Scaler*		scaler_;

    bool			gtIsEmpty() const override
				{ return arrays_.isEmpty(); }
    size_type			gtNrArrays() const override
				{ return arrays_.size(); }
    const ArrayND<float>*	gtArrayData( idx_type iarr ) const override
				{ return arrays_.validIdx(iarr)
				       ? arrays_[iarr] : 0; }
    glob_idx_type		fndNearest(const TrcKey&) const;
    virtual void		gtTrcKey(glob_idx_type,TrcKey&) const	= 0;
    virtual glob_idx_type	gtGlobalIdx(const TrcKey&) const	= 0;

    float			gtNrKBytes() const override;
    void			doDumpInfo(IOPar&) const override;

};
