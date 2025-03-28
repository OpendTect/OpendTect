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
class TrcKeyZSampling;
class UnitOfMeasure;
namespace ZDomain { class Info; }


/*!\brief Base class for all DataPack implementations with a ZDomain.
	  The ZDomain may apply to the datapack space (axis/axes),
	  or to the values stored in the underlying values of the datapack
	  The zDomain() might not be relevant in a very small minority of the
	  datapacks, like in an F-K spectrum, but it will still be set.
	  Will default to SI().zDomainInfo() if not set.
*/

mExpClass(General) ZDataPack : public DataPack
{
public:

    virtual ZDataPack&		setZDomain(const ZDomain::Info&);
    ZDataPack&			setZDomain(const ZDataPack&);
    virtual const ZDomain::Info& zDomain() const { return *zdomaininfo_; }

    const UnitOfMeasure*	zUnit() const;
    bool			zIsTime() const;
    bool			zInMeter() const;
    bool			zInFeet() const;

protected:
				ZDataPack(const char* categry);
				ZDataPack(const ZDataPack&);
				~ZDataPack();

    const ZDomain::Info&	zDomain(bool display) const;
    const UnitOfMeasure*	zUnit(bool display) const;

private:
    const ZDomain::Info*	zdomaininfo_;
};


/*!\brief DataPack for point data. */

mExpClass(General) PointDataPack : public ZDataPack
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

mExpClass(General) FlatDataPack : public ZDataPack
{
public:
				FlatDataPack(const char* categry,
					     Array2D<float>*);
				//!< Array2D become mine (of course)
				FlatDataPack(const FlatDataPack&);

    bool			isOK() const override;
    virtual Array2D<float>&	data()			{ return *arr2d_; }
    const Array2D<float>&	data() const;

    virtual float		getPosDistance( bool dim0, float posfidx ) const
				{ return mUdf(float); }

    virtual FlatPosData&	posData()		{ return posdata_; }
    const FlatPosData&		posData() const;

    virtual uiString		dimName(bool dim0) const;
    virtual uiString		dimUnitLbl(bool dim0,bool display,
					   bool abbreviated=true,
					   bool withparentheses=true) const;
    virtual const UnitOfMeasure* dimUnit( bool dim0, bool display ) const
				 { return nullptr; }

    virtual TrcKey		getTrcKey(int,int) const
				{ return TrcKey::udf(); }
    virtual Coord3		getCoord(int,int) const;
				//!< int,int = Array2D position
    virtual double		getZ(int,int) const	{ return mUdf(double); }
				//!< overload to set Z also in getCoord

    virtual bool		isVertical() const	{ return true; }
    virtual bool		posDataIsCoord() const	{ return true; }
				// Alternative positions for dim0
    virtual void		getAltDimKeys(uiStringSet&,bool dim0) const {}
				//!< First one is 'default'
    virtual void		getAltDimKeysUnitLbls(uiStringSet&,bool dim0,
					bool abbreviated=true,
					bool withparentheses=true) const    {}
    virtual double		getAltDimValue(int ikey,bool dim0,
					       int idim0) const;
    virtual bool		dimValuesInInt(const uiString& key,
					       bool dim0) const
				{ return false; }

    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const;

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
    void			setDimNames(const uiString&,const uiString&,
					    bool forxy);
    uiString			dimName(bool dim0) const override;
    uiString			dimUnitLbl(bool dim0,bool display,
					   bool abbreviated=true,
				      bool withparentheses=true) const override;
    const UnitOfMeasure*	dimUnit(bool dim0,bool display) const override;
    bool			dimValuesInInt(const uiString& key,
					       bool dim0) const override;

    bool			isVertical() const override { return false; }
				//!< Alternatively, it can be in Inl/Crl
    bool			posDataIsCoord() const override
				{ return isposcoord_; }
    TrcKey			getTrcKey(int,int) const override;
    double			getZ(int,int) const override;
    void			setPosCoord(bool yn);
				//!< int,int = Array2D position
    void			setProps(StepInterval<double> inlrg,
					 StepInterval<double> crlrg,
					 bool isposcoord,const uiStringSet*);
    void			setZVal(double);
				/*!< pass -mUdf(double) if the array values are
				     the Z values	*/
    void			initXYRotArray(TaskRunner* =nullptr);

    void			setRange( StepInterval<double> dim0rg,
					  StepInterval<double> dim1rg,
					  bool forxy );

protected:
				~MapDataPack();

    float			getValAtIdx(int,int) const;
    friend class		MapDataPackXYRotator;

    Array2D<float>*		xyrotarr2d_	= nullptr;
    FlatPosData&		xyrotposdata_;
    double			zval_		= mUdf(double);
    bool			isposcoord_	= false;
    uiStringSet			axeslbls_;
    ObjectSet<const UnitOfMeasure> axesunits_;
    Threads::Lock		initlock_;
};



/*!\brief DataPack for volume data, where the dims correspond to
	  inl/crl/z . */

mExpClass(General) VolumeDataPack : public ZDataPack
{ mODTextTranslationClass(VolumeDataPack)
public:

    virtual bool		is2D() const				= 0;
    virtual int			nrTrcs() const				= 0;
    virtual ZSampling		zRange() const				= 0;
    virtual TrcKey		getTrcKey(int globaltrcidx) const	= 0;
    virtual int			getGlobalIdx(const TrcKey&) const	= 0;
    virtual int			getNearestGlobalIdx(const TrcKey&) const;
    virtual bool		isRegular() const	{ return false; }
    virtual bool		isRandom() const	{ return false; }

    void			getPath(TrcKeySet&) const;

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

    static const char*		categoryStr(const TrcKeyZSampling&);
    static const char*		categoryStr(bool isvertical,bool is2d);

    const Array3DImpl<float>&	data(int component=0) const;
    Array3DImpl<float>&		data(int component=0);

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
    void			setRandomLineID(const RandomLineID&);
    RandomLineID		getRandomLineID() const;

    int				getComponentIdx(const char* nm,
						int defcompidx=-1) const;

protected:
				VolumeDataPack(const char*,const BinDataDesc*);
				~VolumeDataPack();

    bool			addArray(int sz0,int sz1,int sz2);
    bool			addArrayNoInit(int sz0,int sz1,int sz2);

    BufferStringSet			componentnames_;
    ObjectSet<Array3DImpl<float> >	arrays_;
    TypeSet<float>			refnrs_;
    BinDataDesc				desc_;
    const Scaler*			scaler_ = nullptr;
    const UnitOfMeasure*		valunit_ = nullptr;
    RandomLineID			rdlid_;
};

// Do not use, only for legacy code
typedef VolumeDataPack SeisDataPack;
