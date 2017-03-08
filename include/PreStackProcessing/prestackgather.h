#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "arrayndimpl.h"
#include "datapackbase.h"
#include "dbkey.h"
#include "offsetazimuth.h"
#include "position.h"
#include "samplingdata.h"

class IOObj;
class SeisPSReader;
class SeisTrc;
class SeisTrcBuf;

namespace PreStack
{

/*!\brief PreStack gather. */

mExpClass(PreStackProcessing) Gather : public FlatDataPack
{ mODTextTranslationClass(Gather)
public:
				Gather();
				Gather(const FlatPosData&);
				mDeclMonitorableAssignment(Gather);

    bool			is3D() const	{ return !trckey_.is2D(); }

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

    const TrcKey&		getTrcKey() const	{ return trckey_; }
    void			setTrcKey(const TrcKey& tk )
				{ trckey_ = tk; }

    const DBKey&		getStoredID() const	{ return storageid_; }
    const StepInterval<float>&	zRange() const		{ return zrg_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zrg_ = zrg; }

    bool			isLoaded() const	{ return arr2d_; }

    const char*			dimName(bool dim0) const;
    void			getAuxInfo(int,int,IOPar&) const;

    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    float			getOffset(int) const;
    float			getAzimuth(int) const;
    OffsetAzimuth		getOffsetAzimuth(int) const;

    bool			isOffsetAngle() const	{return offsetisangle_;}
    bool			isCorrected() const	{ return iscorr_; }
    void			setCorrected(bool yn)	{ iscorr_ = yn; }
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
    const BinID&		getBinID() const
				{ return trckey_.position(); }
    void			setBinID( const BinID& bid )
				{ trckey_.setPosition( bid ); }

protected:

				~Gather();

    DBKey			velocityid_;
    DBKey			storageid_;
    DBKey			staticsid_;
    bool			offsetisangle_;
    bool			iscorr_;

    bool			zit_;
    TrcKey			trckey_;
    Coord			coord_;
    TypeSet<float>		azimuths_;
    StepInterval<float>		zrg_;

public:

    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					    bool snapzrangetosi=false);
};


/*!\brief A DataPack containing an objectset of gathers. */

mExpClass(PreStackProcessing) GatherSetDataPack : public DataPack
{
public:
				GatherSetDataPack(const char* ctgery,
						  const ObjectSet<Gather>&);
				mDeclMonitorableAssignment(GatherSetDataPack);

    void			fill(Array2D<float>&,int offsetidx) const;
    void			fill(SeisTrcBuf&,int offsetidx) const;
    void			fill(SeisTrcBuf&,Interval<float> stackrg) const;
    SeisTrc*			getTrace(int gatheridx,int offsetidx);
    const SeisTrc*		getTrace(int gatheridx,int offsetidx) const;

    virtual float		nrKBytes() const	{ return 0; }

    const Gather*		getGather(const BinID&) const;
    const ObjectSet<Gather>&	getGathers() const	{ return gathers_; }
    ObjectSet<Gather>&		getGathers()		{ return gathers_; }
    void			setGathers( RefObjectSet<Gather>& gathers )
				{ gathers_ = gathers;}

protected:

				~GatherSetDataPack();

    SeisTrc*			gtTrace(int gatheridx,int offsetidx) const;

    RefObjectSet<Gather>	gathers_;

};

} // namespace PreStack
