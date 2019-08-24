#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "arrayndimpl.h"
#include "datapackbase.h"
#include "dbkey.h"
#include "enums.h"
#include "offsetazimuth.h"
#include "position.h"
#include "samplingdata.h"

class SeisPSReader;
class SeisTrc;
class SeisTrcBuf;
class TrcKey;


/*!\brief PreStack gather. */

mExpClass(Seis) Gather : public FlatDataPack
{ mODTextTranslationClass(Gather)
public:
				mTypeDefArrNDTypes;

    enum Type			{ Off, Ang };
				mDeclareEnumUtils(Type)
    enum Unit			{ Meter, Feet, Deg, Rad, None };
				mDeclareEnumUtils(Unit)

				Gather();
				Gather(const FlatPosData&);
				mDeclMonitorableAssignment(Gather);

    bool			is3D() const;

    void			getFlatPosData(FlatPosData&) const;

    bool			readFrom(const DBKey&,const TrcKey&,
					 int component=0,
					 uiString* errmsg=0);
    bool			readFrom(const IOObj&,const TrcKey&,
					 int component=0,
					 uiString* errmsg=0);
    bool			readFrom(const IOObj&,SeisPSReader& rdr,
					 const TrcKey&,int component=0,
					 uiString* errmsg=0);

    const Coord&		getCoord() const	{ return coord_; }
    virtual Coord3		getCoord(int,int) const
				{ return Coord3(coord_.x_,coord_.y_,0); }

    void                        detectOuterMutes(int* depths,
						 int taperlen=0) const;
				/*!<For each trace, find the depth where the
				    last outer-mute-affecte value is*/
    void                        detectInnerMutes(int* depths,
						 int taperlen=0) const;
	                        /*<!For each trace, try to detect the first
				   inner-mute affected value. */

    const DBKey&		getStoredID() const	{ return storageid_; }
    const StepInterval<float>&	zRange() const		{ return zrg_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zrg_ = zrg; }

    bool			isLoaded() const	{ return arr2d_; }
    int				nrOffsets() const
				{ return data().getSize( 0 ); }

    const char*			dimName(bool dim0) const;
    void			getAuxInfo(int,int,IOPar&) const;

    static dim_idx_type		offsetDim()		{ return 0; }
    static dim_idx_type		zDim()			{ return 1; }

    float			getOffset(int) const;
    float			getAzimuth(int) const;
    OffsetAzimuth		getOffsetAzimuth(int) const;

    Seis::DataType		ampType() const		{ return amptype_; }
    Unit			ampUnit() const		{ return ampunit_; }
    void			setAmpType(Seis::DataType,Unit=None);
    bool			isOffsetAngle() const;
    void			setIsOffsetAngle(bool yn,Unit=Deg);
    Unit			getXAxisUnit() const	{ return xaxisunit_;}
    bool			isCorrected() const	{ return iscorr_; }
    void			setCorrected( bool yn ) { iscorr_ = yn; }
    bool			zIsTime() const		{ return zit_; }

    const DBKey&		getVelocityID() const	{ return velocityid_; }
    const DBKey&		getStorageID() const    { return storageid_; }
    const DBKey&		getStaticsID() const	{ return staticsid_; }

    static bool			getVelocityID(const DBKey& stor,DBKey& vid);

    static const char*		sDataPackCategory();
    static const char*		sKeyIsAngleGather();
    static const char*		sKeyIsCorr();
    static const char*		sKeyZisTime();

    static const char*		sKeyPostStackDataID();
    static const char*		sKeyStaticsID();

    void			getAzimuths( TypeSet<float>& azimuths ) const
				{ azimuths = azimuths_; }
    void			setAzimuths( const TypeSet<float>& azimuths )
				{ azimuths_ = azimuths; }

				// Will be removed after this version.
    mDeprecated bool		readFrom(const DBKey&,const BinID&,
					 int component=0,
					 uiString* errmsg=0);
    mDeprecated bool		readFrom(const IOObj&,const BinID&,
					 int component=0,
					 uiString* errmsg=0);
    mDeprecated bool		readFrom(const IOObj&,SeisPSReader& rdr,
					 const BinID&,int component=0,
					 uiString* errmsg=0);
    mDeprecated bool		readFrom(const DBKey&, const int tracenr,
					 const char* linename,int comp,
					 uiString* errmsg=0);
    mDeprecated bool		readFrom(const IOObj&, const int tracenr,
					 const char* linename,int comp,
					 uiString* errmsg=0);

    BinID			getBinID() const;
    void			setBinID(const BinID&);
    Bin2D			getBin2D() const;
    void			setBin2D(const Bin2D&);
    const TrcKey&		getTrcKey() const	{ return trckey_; }
    void			setTrcKey(const TrcKey&);

protected:

    virtual			~Gather();

    TrcKey&			trckey_;
    DBKey			velocityid_;
    DBKey			storageid_;
    DBKey			staticsid_;
    Type			type_;
    Seis::DataType		amptype_;
    Unit			ampunit_;
    Unit			xaxisunit_;
    bool			iscorr_;

    bool			zit_;
    Coord			coord_;
    TypeSet<float>		azimuths_;
    StepInterval<float>		zrg_;

    virtual void		doDumpInfo(IOPar&) const;

public:

    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					    bool snapzrangetosi=false);
};


/*!\brief A DataPack containing an objectset of gathers. */

mExpClass(Seis) GatherSetDataPack : public DataPack
{
public:

    typedef ObjectSet<Gather>	GatherSet;

				GatherSetDataPack()
				    : DataPack(sDataPackCategory())	{}
				GatherSetDataPack(const GatherSet&);
				mDeclMonitorableAssignment(GatherSetDataPack);
    bool			isEmpty() const { return gathers_.isEmpty(); }

    void			fill(Array2D<float>&,int offsetidx) const;
    void			fill(SeisTrcBuf&,int offsetidx) const;
    void			fill(SeisTrcBuf&,Interval<float> stackrg) const;
    bool			fillGatherBuf(SeisTrcBuf&,const BinID&) const;
    bool			fillGatherBuf(SeisTrcBuf&,const Bin2D&) const;
    SeisTrc*			createTrace(int gatheridx,int offsetidx) const;
    SeisTrc*			createTrace(const BinID&,int offsetidx) const;
    SeisTrc*			createTrace(const Bin2D&,int offsetidx) const;

    const Gather*		getGather(const BinID&) const;
    const Gather*		getGather(const Bin2D&) const;
    const GatherSet&		getGathers() const	{ return gathers_; }
    GatherSet&			getGathers()		{ return gathers_; }
    void			addGather(Gather*);
    void			setGathers( const GatherSet& gathers )
				{ gathers_ = gathers; }

    ZSampling			zRange() const;

    static const char*		sDataPackCategory();

protected:

	virtual			~GatherSetDataPack();

    SeisTrc*			crTrace(int gatheridx,int offsetidx) const;

    RefObjectSet<Gather>	gathers_;

    virtual bool		gtIsEmpty() const { return gathers_.isEmpty(); }
    virtual float		gtNrKBytes() const;
    virtual void		doDumpInfo(IOPar&) const;

public:

    mDeprecated SeisTrc*	getTrace( int gid, int oi ) const
				{ return createTrace( gid, oi ); }
    mDeprecated SeisTrc*	getTrace( const BinID& bid, int oi ) const
				{ return createTrace( bid, oi ); }

};
