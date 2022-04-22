#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emhorizon.h"
#include "binidsurface.h"
#include "task.h"

class BinIDValueSet;
class DataPointSet;
class BufferStringSet;
class TrcKeySampling;
class Scaler;
class ZAxisTransform;
namespace Pick { class Set; }
namespace Pos { class Provider3D; }
namespace Table { class FormatDesc; }
namespace Threads { class WorkManager; }

namespace EM
{
class SurfaceAuxData;

/*!
\brief 3D HorizonGeometry
*/

mExpClass(EarthModel) Horizon3DGeometry : public HorizonGeometry
{ mODTextTranslationClass(Horizon3DGeometry);
public:
				Horizon3DGeometry(Surface&);

    const Geometry::BinIDSurface* sectionGeometry(
					const SectionID&) const override;
    Geometry::BinIDSurface*	sectionGeometry(const SectionID&) override;

    bool			removeSection(const SectionID&,
					      bool hist) override;
    SectionID			cloneSection(const SectionID&) override;

    bool			isFullResolution() const override;
    RowCol			loadedStep() const;
    RowCol			step() const;
    void			setStep(const RowCol& step,
					const RowCol& loadedstep);

    PosID			getPosID(const TrcKey&) const override;
    TrcKey			getTrcKey(const PosID&) const override;

    bool			enableChecks(bool yn) override;
    bool			isChecksEnabled() const override;
    bool			isNodeOK(const PosID&) const override;

    bool			isAtEdge(const PosID& pid) const override;
    PosID			getNeighbor(const PosID&,const RowCol&) const;
    int				getConnectedPos(const PosID&,
						TypeSet<PosID>*) const override;

    bool			getBoundingPolygon(const SectionID&,
						   Pick::Set&) const;
    void			getDataPointSet( const SectionID&,
						  DataPointSet&,
						  float shift=0.0) const;
    void			fillBinIDValueSet(const SectionID&,
						 BinIDValueSet&,
						 Pos::Provider3D* prov=0) const;

    EMObjectIterator*		createIterator(const EM::SectionID&,
				   const TrcKeyZSampling* =0) const override;
protected:

    Geometry::BinIDSurface*	createSectionGeometry() const override;

    RowCol			loadedstep_;
    RowCol			step_;
    bool			checksupport_;
};


/*!
\brief 3D Horizon. A Horizon is made up of one or more grids (so they can
overlap at faults). The grids are defined by knot-points in a matrix and
fillstyle in between the knots.
*/

mExpClass(EarthModel) Horizon3D : public Horizon
{ mDefineEMObjFuncs( Horizon3D );
public:

    float			getZ(const TrcKey&) const override;
				//!< Fast: reads from the first section
    bool			setZ(const TrcKey&,float z,
				     bool addtohist) override;
				//!< Fast: writes to the first section
    bool			setZAndNodeSourceType(const TrcKey&,
					float z,bool addtohist,
					NodeSourceType type=Auto) override;

    void			setNodeSourceType(const TrcKey&,
						  NodeSourceType) override;
    void			setNodeSourceType(const PosID&,NodeSourceType);
    bool			isNodeSourceType(const PosID&,
						 NodeSourceType) const override;
    bool			isNodeSourceType(const TrcKey&,
						 NodeSourceType) const override;
    bool			isNodeLocked(const PosID&)const override;

    bool			hasNodeSourceType(const PosID&) const override;

    bool			hasZ(const TrcKey&) const override;
				//!< Fast: checks only the first section
    Coord3			getCoord(const TrcKey&) const override;
    void			setAttrib(const TrcKey&,int attr,int yn,
					  bool addtohist) override;
    bool			isAttrib(const TrcKey&,int attr) const override;

    float			getZValue(const Coord&,bool allow_udf=true,
					  int nr=0) const override;
				//!< Slow: if you need the choices

    TrcKeySampling		range() const;
    Interval<float>		getZRange() const;

    void			removeAll() override;
    Horizon3DGeometry&		geometry() override;
    const Horizon3DGeometry&	geometry() const override;

    virtual void		setArray(const SectionID&,const BinID& start,
					const BinID& step, Array2D<float>* arr,
					bool takeover);
    static Horizon3D*		createWithConstZ(float z,const TrcKeySampling&);
    Array2D<float>*		createArray2D(SectionID,
					      const ZAxisTransform* zt=0) const;
    bool			setArray2D(const Array2D<float>&,SectionID,
					   bool onlyfillundefs,
					   const char* histdesc,bool trimundef);
				/*!< Returns true on succes.  If histdesc
				     is set, the event will be saved to
				     history with the desc. */
    bool			setArray2D(Array2D<float>*,const BinID& origin,
					   const BinID& step,
					   bool takeoverarr=true);
				/*!< Returns true on succes. Takes over array
				     when takeoverarr=true.
				     Removes any existing data. */

    Executor*			importer(const ObjectSet<BinIDValueSet>&,
					 const TrcKeySampling& hs);
					/*!< Removes all data and creates
					  a section for every BinIDValueSet
					*/
    Executor*			auxDataImporter(const ObjectSet<BinIDValueSet>&,
					const BufferStringSet& attribnms,int,
					const TrcKeySampling& hs);


    Pos::GeomID			getSurveyGeomID() const { return survgeomid_; }
				//!A 3D Horizon is locked to one survey
				//!Geometry
    void			setSurveyGeomID(Pos::GeomID);

    uiString			getUserTypeStr() const override
				{ return userTypeStr(); }
    static uiString		userTypeStr()
				{ return tr("3D Horizon"); }

    void			initAllAuxData(float val=mUdf(float));
    SurfaceAuxData&		auxdata;

    void			initTrackingAuxData(float val=mUdf(float));
    void			initTrackingArrays();
    void			updateTrackingSampling();
    bool			saveParentArray();
    bool			readParentArray();
    bool			saveNodeArrays();
    bool			readNodeArrays();

    TrcKeySampling		getTrackingSampling() const;
    void			setParent(const TrcKey&,const TrcKey& parent);
    TrcKey			getParent(const TrcKey&) const;
    void			getParents(const TrcKey&,
					   TypeSet<TrcKey>&) const;
    bool			selectChildren(const TrcKey&);
    Array2D<char>*		getChildren() const;
    void			deleteChildren();
    void			resetChildren();
    void			setNodeLocked(const TrcKey&,
					      bool locked) override;
    bool			isNodeLocked(const TrcKey&) const override;
    void			lockAll() override;
    void			unlockAll() override;
    const Array2D<char>*	getLockedNodes() const override;

    void			setParentColor(const OD::Color&);
    const OD::Color&		getParentColor() const;

    bool			setPos(const EM::PosID&,const Coord3&,
				       bool addtohistory) override;
    bool			setPos(const EM::SectionID&,const EM::SubID&,
				   const Coord3&,bool addtohistory) override;
    void			apply(const Pos::Filter&) override;

protected:
    enum			ArrayType{Parents,Children,LockNode,NodeSource};

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;
    void			fillDisplayPar(IOPar&) const override;
    bool			useDisplayPar(const IOPar&) override;

    const IOObjContext&		getIOObjContext() const override;
    void			initNodeArraysSize(const StepInterval<int>&,
						   const StepInterval<int>&);
    void			setNodeArraySize(const StepInterval<int>&,
				     const StepInterval<int>&,ArrayType);
    void			updateNodeSourceArray(const TrcKeySampling,
						      ArrayType);
    Array2D<char>*		getNodeSourceArray(ArrayType) const;
    void			setNodeSourceArray(Array2D<char>*,ArrayType);
    void			createNodeSourceArray(const StepInterval<int>&,
						const StepInterval<int>&,
						ArrayType);
    TrcKeySampling		getSectionTrckeySampling() const;

    friend class		EMManager;
    friend class		EMObject;
    Horizon3DGeometry		geometry_;

    TrcKeySampling		trackingsamp_;
    Array2D<char>*		lockednodes_;
    Array2D<char>*		children_;
    Array2D<od_int64>*		parents_;

    OD::Color			parentcolor_;

    Pos::GeomID			survgeomid_;
    Array2D<char>*		nodesource_;
				/*!< '0'- non interpreted, '1'- manual
				interpreted,'2' - auto interpreted.
				'3'-Gridding. see enum NodeSourceType*/
    bool			arrayinited_;

public:
    /*mDeprecated*/ float	getZ(const BinID&) const;
				//!< Fast: reads from the first section
    /*mDeprecated*/ bool	setZ(const BinID&,float z,bool addtohist);
				//!< Fast: writes to the first section
    Array2D<char>*		getChildren(const TrcKey&) const
				{ return getChildren(); }
    OD::GeomSystem		getSurveyID() const override
				{ return geomSystemOf(getSurveyGeomID()); }

    static OD::Color		sDefaultSelectionColor();
    static OD::Color		sDefaultLockColor();
};


mExpClass(EarthModel) ChildFinder : public SequentialTask
{
friend class FindTask;
friend class Horizon3D;
protected:
			ChildFinder(const TrcKeySampling& tks,
				    const Array2D<od_int64>& parents,
				    Array2D<char>& children );
			~ChildFinder();


    void		addTask(od_int64);
    void		taskFinished(CallBacker*);
    int			nextStep() override;

    Threads::WorkManager&	twm_;
    int				queueid_;
    const Array2D<od_int64>&	parents_;
    Array2D<char>&		children_;
    TrcKeySampling		tks_;

    Threads::Atomic<int>	nrtodo_;
    Threads::Atomic<int>	nrdone_;

    Threads::Lock		addlock_;
    Threads::Lock		finishlock_;
};

} // namespace EM

