#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emfault.h"
#include "emmanager.h"
#include "executor.h"

namespace Geometry { class FaultStickSet; }
namespace Pos { class Filter; }

namespace EM
{

mExpClass(EarthModel) FaultSSDataHolder
{
public:
						FaultSSDataHolder();
						~FaultSSDataHolder();
    ObjectSet<const Geometry::FaultStick>	sticks_;
    Pos::GeomID					geomid_;
    TypeSet<int>				sticknr_;
    TrcKeyZSampling				tkzs_;
};

/*!
\brief FaultStickSet geometry.
*/

mExpClass(EarthModel) FaultStickSetGeometry : public FaultGeometry
{
public:
			FaultStickSetGeometry(Surface&);
			~FaultStickSetGeometry();

    int			nrSticks() const;
    int			nrKnots(int sticknr) const;

    bool		insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory) override;
    bool		insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    const MultiID* pickedmid,
				    const char* pickednm,bool addtohistory);
    bool		insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    Pos::GeomID pickedgeomid,bool addtohistory);
    bool		removeStick(int sticknr,
				    bool addtohistory) override;
    bool		insertKnot(const SubID&,
			       const Coord3& pos,bool addtohistory) override;
    bool		removeKnot(const SubID&,
				   bool addtohistory) override;

    bool		pickedOnPlane(int sticknr) const;
    bool		pickedOn2DLine(int sticknr) const;
    bool		pickedOnHorizon(int sticknr) const;

    const MultiID*	pickedMultiID(int sticknr) const override;
    const char*		pickedName(int sticknr) const override;
    Pos::GeomID		pickedGeomID(int sticknr)const;

    ObjectType		FSSObjType() const;


    Geometry::FaultStickSet* geometryElement() override;
    const Geometry::FaultStickSet* geometryElement() const override;

    EMObjectIterator*	createIterator(
				const TrcKeyZSampling* =nullptr) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
    const ObjectSet<FaultSSDataHolder>& getDataHodlers() const 
						{ return dataholders_; }
    inline const Interval<float>&	getZRange() const { return zgate_; }

// Deprecated public functions
    mDeprecated("Use without SectionID")
    int			nrSticks(const SectionID&) const
			{ return nrSticks(); }
    mDeprecated("Use without SectionID")
    int			nrKnots(const SectionID&,int sticknr) const
			{ return nrKnots(sticknr); }

    mDeprecated("Use without SectionID")
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory) override
			{ return insertStick(sticknr,firstcol,pos,editnormal,
					     addtohistory); }
    mDeprecated("Use without SectionID")
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    const MultiID* pickedmid,
				    const char* pickednm,bool addtohistory)
			{ return insertStick(sticknr,firstcol,pos,editnormal,
					     pickedmid,pickednm,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    Pos::GeomID pickedgeomid,bool addtohistory)
			{ return insertStick(sticknr,firstcol,pos,editnormal,
					     pickedgeomid,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		removeStick(const SectionID&,int sticknr,
				    bool addtohistory) override
			{ return removeStick(sticknr,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		insertKnot(const SectionID&,const SubID& subid,
			       const Coord3& pos,bool addtohistory) override
			{ return insertKnot(subid,pos,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		removeKnot(const SectionID&,const SubID& subid,
				   bool addtohistory) override
			{ return removeKnot(subid,addtohistory); }

    mDeprecated("Use without SectionID")
    bool		pickedOnPlane(const SectionID&,int sticknr) const
			{ return pickedOnPlane(sticknr); }
    mDeprecated("Use without SectionID")
    bool		pickedOn2DLine(const SectionID&,int sticknr) const
			{ return pickedOn2DLine(sticknr); }
    mDeprecated("Use without SectionID")
    bool		pickedOnHorizon(const SectionID&,int sticknr) const
			{ return pickedOnHorizon(sticknr); }

    mDeprecated("Use without SectionID")
    const MultiID*	pickedMultiID(const SectionID&,
				      int sticknr) const override
			{ return pickedMultiID(sticknr); }
    mDeprecated("Use without SectionID")
    const char*		pickedName(const SectionID&,int sticknr) const override
			{ return pickedName(sticknr); }
    mDeprecated("Use without SectionID")
    Pos::GeomID		pickedGeomID(const SectionID&,int sticknr)const
			{ return pickedGeomID(sticknr); }

    mDeprecated("Use geometryElement()")
    Geometry::FaultStickSet*
			sectionGeometry(const SectionID&) override
			{ return geometryElement(); }
    mDeprecated("Use geometryElement() const")
    const Geometry::FaultStickSet*
			sectionGeometry(const SectionID&) const override
			{ return geometryElement(); }

protected:
    Geometry::FaultStickSet*	createGeometryElement() const override;
    int			indexOf(int sticknr) const;

    mDeprecated("Use without SectionID")
    int			indexOf(const SectionID& sid,int sticknr) const
			{ return indexOf(sticknr); }

    struct StickInfo
    {
				StickInfo();

	SectionID		sid = SectionID::def();
	int			sticknr		= mUdf(int);
	Pos::GeomID		pickedgeomid;
	MultiID			pickedmid;
	BufferString		pickednm;
    };

    ObjectSet<StickInfo>		stickinfo_;
    ObjectSet<FaultSSDataHolder>	dataholders_;
    Interval<float>			zgate_;
};


/*!
\brief Fault stick set.
*/

mExpClass(EarthModel) FaultStickSet: public Fault
{ mDefineEMObjFuncs( FaultStickSet );
public:
    FaultStickSetGeometry&		geometry() override;
    const FaultStickSetGeometry&	geometry() const override;
    void				apply(const Pos::Filter&) override;
    uiString				getUserTypeStr() const override;


    EMObjectIterator*		createIterator(
				const TrcKeyZSampling* =nullptr) const override;

protected:

    const IOObjContext&			getIOObjContext() const override;

    FaultStickSetGeometry		geometry_;
};



mExpClass(EarthModel) FaultStickSetDataOrganiser : public Executor
{ mODTextTranslationClass(FaultStickSetDataOrganiser)
public:
		       FaultStickSetDataOrganiser(const FaultStickSetGeometry&,
						 ObjectSet<FaultSSDataHolder>&);
		       ~FaultStickSetDataOrganiser();

    uiString			    uiMessage() const override { return msg_; }
    uiString			    uiNrDoneText() const override;
    od_int64			    nrDone() const override { return nrdone_; }
    od_int64			    totalNr() const override { return totnr_; }
protected:


    const EM::FaultStickSetGeometry&	fssgeom_;
    uiString				msg_;
    od_int64				nrdone_		    = 0;
    od_int64				totnr_		    = 0;
    int					nextStep() override;

    ObjectSet<FaultSSDataHolder>&	dataholders_;
    TypeSet<Pos::GeomID>		processedgeomids_;

};

mExpClass(EarthModel) FaultStickSetDataUpdater : public ParallelTask
{ mODTextTranslationClass(FaultStickSetDataUpdater)
public:
			    FaultStickSetDataUpdater(Geometry::FaultStickSet&,
						 ObjectSet<FaultSSDataHolder>&);
			    ~FaultStickSetDataUpdater();

protected:
    bool			    doWork(od_int64,od_int64,int);
    od_int64			    nrIterations() const override;
    Geometry::FaultStickSet&	    faultstickset_;
    ObjectSet<FaultSSDataHolder>&   dataholders_;
    od_int64			    totnr_		= 0;
};

} // namespace EM
