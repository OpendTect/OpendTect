#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "arrayndimpl.h"
#include "bindatadesc.h"
#include "bufstringset.h"
#include "datapack.h"
#include "integerid.h"
#include "position.h"
#include "trckeysampling.h"
#include "valseries.h"

class FlatPosData;
class Scaler;
class TaskRunner;
class UnitOfMeasure;
namespace ZDomain { class Info; }


/*!\brief DataPack for point data. */

mExpClass(General) PointDataPack : public DataPack
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

				PointDataPack(const char* categry);
				~PointDataPack();

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
				FlatDataPack(const FlatDataPack&);

    bool			isOK() const override	{ return arr2d_; }
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

    virtual bool		posDataIsCoord() const	{ return true; }
				// Alternative positions for dim0
    virtual void		getAltDim0Keys(BufferStringSet&) const	{}
				//!< First one is 'default'
    virtual double		getAltDim0Value(int ikey,int idim0) const;
    virtual bool		dimValuesInInt(const char* key) const
				{ return false; }

    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const {}

    float			nrKBytes() const override;
    void			dumpInfo(StringPairSet&) const override;

    virtual int			size(bool dim0) const;

protected:

				FlatDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor
				~FlatDataPack();

    Array2D<float>*		arr2d_ = nullptr;
    FlatPosData&		posdata_;

private:

    void			init();

};


/*!\brief DataPack for 2D data to be plotted on a Map. */

mExpClass(General) MapDataPack : public FlatDataPack
{
public:
				MapDataPack(const char* cat,Array2D<float>*);

    Array2D<float>&		data() override;
    FlatPosData&		posData() override;
    const Array2D<float>&	rawData() const		{ return *arr2d_; }
    const FlatPosData&		rawPosData() const	{ return posdata_; }
    void			setDimNames(const char*,const char*,bool forxy);
    const char*			dimName(bool dim0) const override;

				//!< Alternatively, it can be in Inl/Crl
    bool			posDataIsCoord() const override
				{ return isposcoord_; }
    void			setPosCoord(bool yn);
				//!< int,int = Array2D position
    void			getAuxInfo(int idim0,
					   int idim1,IOPar&) const override;
    void			setProps(StepInterval<double> inlrg,
					 StepInterval<double> crlrg,
					 bool,BufferStringSet*);
    void			initXYRotArray(TaskRunner* = 0 );

    void			setRange( StepInterval<double> dim0rg,
					  StepInterval<double> dim1rg,
					  bool forxy );

protected:
				~MapDataPack();

    float			getValAtIdx(int,int) const;
    friend class		MapDataPackXYRotator;

    Array2D<float>*		xyrotarr2d_ = nullptr;
    FlatPosData&		xyrotposdata_;
    bool			isposcoord_ = false;
    TypeSet<BufferString>	axeslbls_;
    Threads::Lock		initlock_;
};



/*!\brief DataPack for volume data, where the dims correspond to
	  inl/crl/z . */

mExpClass(General) VolumeDataPack : public DataPack
{
public:

    virtual Array3D<float>&	data();
    const Array3D<float>&	data() const;

    virtual const char*		dimName(char dim) const;
    virtual double		getPos(char dim,int idx) const;
    int				size(char dim) const;
    float			nrKBytes() const override;
    void			dumpInfo(StringPairSet&) const override;


protected:
				VolumeDataPack(const char* categry,
					     Array3D<float>*);
				//!< Array3D become mine (of course)
				~VolumeDataPack();

				VolumeDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    Array3D<float>*		arr3d_ = nullptr;
};



/*!\brief DataPack for volume data. Should be renamed to VolumeDataPack later*/

mExpClass(General) SeisDataPack : public DataPack
{ mODTextTranslationClass(SeisDataPack)
public:

    virtual bool		is2D() const				= 0;
    virtual int			nrTrcs() const				= 0;
    virtual ZSampling		zRange() const				= 0;
    virtual TrcKey		getTrcKey(int globaltrcidx) const	= 0;
    virtual int			getGlobalIdx(const TrcKey&) const	= 0;
    virtual int			getNearestGlobalIdx(const TrcKey&) const;

    void			getPath(TrcKeyPath&) const;

    virtual bool		addComponent(const char* nm)		= 0;


    const OffsetValueSeries<float> getTrcStorage(
					int comp,int globaltrcidx) const;

    const float*		getTrcData(int comp,int globaltrcidx) const;
    float*			getTrcData(int comp,int globaltrcidx);

    bool			getCopiedTrcData(int comp,int globaltrcidx,
						 Array1D<float>&) const;

    int				nrComponents() const
				{ return arrays_.size(); }
    bool			isEmpty() const
				{ return arrays_.isEmpty(); }
    bool			validComp( int comp ) const
				{ return arrays_.validIdx( comp ); }
    void			setComponentName(const char*,int comp=0);
    const char*			getComponentName(int comp=0) const;

    static const char*		categoryStr(bool isvertical,bool is2d);

    const Array3DImpl<float>&	data(int component=0) const;
    Array3DImpl<float>&		data(int component=0);

    SeisDataPack&		setZDomain(const ZDomain::Info&);
    const ZDomain::Info&	zDomain() const { return *zdomaininfo_; }
    const UnitOfMeasure*	zUnit() const;
    BufferString		unitStr(bool val,bool withparens=false) const;

    const UnitOfMeasure*	valUnit() const { return valunit_; }
    void			setValUnit( const UnitOfMeasure* uom )
				{ valunit_ = uom; }

    void			setScaler(const Scaler&);
    void			deleteScaler();
    const Scaler*		getScaler() const	{ return scaler_; }

    void			setRefNrs( const TypeSet<float>& refnrs )
				{ refnrs_ = refnrs; }
    float			getRefNr(int globaltrcidx) const;

    const BinDataDesc&		getDataDesc() const	{ return desc_; }
    void			setDataDesc(const BinDataDesc&);
				//<! Will remove incompatible arrays if any

    float			nrKBytes() const override;
    void			dumpInfo(StringPairSet&) const override;
    void			setRandomLineID(RandomLineID);
    RandomLineID		getRandomLineID() const;

    int				getComponentIdx(const char* nm,
						int defcompidx=-1) const;

protected:
				SeisDataPack(const char*,const BinDataDesc*);
				~SeisDataPack();

    bool			addArray(int sz0,int sz1,int sz2);
    bool			addArrayNoInit(int sz0,int sz1,int sz2);

    BufferStringSet			componentnames_;
    ObjectSet<Array3DImpl<float> >	arrays_;
    TypeSet<float>			refnrs_;
    const ZDomain::Info*		zdomaininfo_;
    BinDataDesc				desc_;
    const Scaler*			scaler_ = nullptr;
    const UnitOfMeasure*		valunit_ = nullptr;
    RandomLineID			rdlid_;
};
