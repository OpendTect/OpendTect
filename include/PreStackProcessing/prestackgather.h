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
#include "multiid.h"
#include "offsetazimuth.h"
#include "position.h"
#include "samplingdata.h"

class IOObj;
class SeisPSReader;
class SeisTrc;
class SeisTrcBuf;

namespace PreStack
{

/*!
\brief PreStack gather.
*/

mExpClass(PreStackProcessing) Gather : public FlatDataPack
{ mODTextTranslationClass(Gather)
public:
				Gather();
				Gather(const Gather&);
				Gather(const FlatPosData&);
				~Gather();

    bool			is3D() const { return linename_.isEmpty(); }

    bool			readFrom(const MultiID&,const BinID&,
					 int component=0,
					 uiString* errmsg=0);
    bool			readFrom(const IOObj&,const BinID&,
					 int component=0,
					 uiString* errmsg=0);
    bool			readFrom(const IOObj&,SeisPSReader& rdr,
					 const BinID&,int component=0,
					 uiString* errmsg=0);

    const Coord&		getCoord() const	{ return coord_; }
    virtual Coord3		getCoord(int,int) const
				{ return Coord3(coord_.x,coord_.y,0); }

    void                        detectOuterMutes(int* depths,
						 int taperlen=0) const;
				/*!<For each trace, find the depth where the
				    last outer-mute-affecte value is*/
    void                        detectInnerMutes(int* depths,
						 int taperlen=0) const;
	                        /*<!For each trace, try to detect the first
				   inner-mute affected value. */

				//for 3d only
    const BinID&		getBinID() const	{ return binid_; }
    void			setBinID( const BinID& bid )
				{ binid_ = bid; }
    const MultiID&		getStoredID() const	{ return storagemid_; }
    const StepInterval<float>&	zRange() const		{ return zrg_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zrg_ = zrg; }

				//for 2D only.
    bool			readFrom(const MultiID&, const int tracenr,
					 const char* linename,int comp,
					 uiString* errmsg=0);
    bool			readFrom(const IOObj&, const int tracenr,
					 const char* linename,int comp,
					 uiString* errmsg=0);
    int				getSeis2DTraceNr() const { return binid_.crl();}
    const char*			getSeis2DName() const;

    bool			isLoaded() const	{ return arr2d_; }

    const char*			dimName(bool dim0) const;
    void			getAuxInfo(int,int,IOPar&) const;

    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    float			getOffset(int) const;
    float			getAzimuth(int) const;
    OffsetAzimuth		getOffsetAzimuth(int) const;

    bool			isOffsetAngle() const	{return offsetisangle_;}
    void			setOffsetIsAngle(bool yn);
    bool			isCorrected() const	{ return iscorr_; }
    void			setCorrected(bool yn)	{ iscorr_ = yn; }
    bool			zIsTime() const		{ return zit_; }


    const MultiID&		getVelocityID() const	{ return velocitymid_; }
    const MultiID&		getStorageID() const    { return storagemid_; }
    const MultiID&		getStaticsID() const	{ return staticsmid_; }

    static bool			getVelocityID(const MultiID& stor,MultiID& vid);

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

protected:

    MultiID			velocitymid_;
    MultiID			storagemid_;
    MultiID			staticsmid_;
    bool			offsetisangle_;
    bool			iscorr_;

    bool			zit_;
    BinID			binid_;
    Coord			coord_;
    TypeSet<float>		azimuths_;
    StepInterval<float>		zrg_;

    BufferString		linename_;

public:
    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					    bool snapzrangetosi=false);
};


/*!
\brief A DataPack containing an objectset of gathers.
*/

mExpClass(PreStackProcessing) GatherSetDataPack : public DataPack
{
public:
				GatherSetDataPack(const char* ctgery,
						  const ObjectSet<Gather>&);
				~GatherSetDataPack();

    void			fill(Array2D<float>&,int offsetidx) const;
    void			fill(SeisTrcBuf&,int offsetidx) const;
    void			fill(SeisTrcBuf&,Interval<float> stackrg) const;
    SeisTrc*			getTrace(int gatheridx,int offsetidx);
    const SeisTrc*		getTrace(int gatheridx,int offsetidx) const;

    virtual float		nrKBytes() const	{ return 0; }

    const Gather*		getGather(const BinID&) const;
    const ObjectSet<Gather>&	getGathers() const	{ return gathers_; }
    ObjectSet<Gather>&		getGathers()		{ return gathers_; }

    StepInterval<float>		zRange() const;

    static const char*		sDataPackCategory();

protected:
    SeisTrc*			gtTrace(int gatheridx,int offsetidx) const;

    ObjectSet<Gather>		gathers_;
};

} // namespace PreStack

