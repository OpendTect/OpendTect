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

    const Geometry::BinIDSurface* sectionGeometry(const SectionID&) const;
    Geometry::BinIDSurface*	sectionGeometry(const SectionID&);

    bool			removeSection(const SectionID&,bool hist);
    SectionID			cloneSection(const SectionID&);

    bool			isFullResolution() const;
    RowCol			loadedStep() const;
    RowCol			step() const;
    void			setStep(const RowCol& step,
					const RowCol& loadedstep);

    PosID			getPosID(const TrcKey&) const;
    TrcKey			getTrcKey(const PosID&) const;

    bool			enableChecks(bool yn);
    bool			isChecksEnabled() const;
    bool			isNodeOK(const PosID&) const;

    bool			isAtEdge(const PosID& pid) const;
    PosID			getNeighbor(const PosID&,const RowCol&) const;
    int				getConnectedPos(const PosID&,
						TypeSet<PosID>*) const;

    bool			getBoundingPolygon(const SectionID&,
						   Pick::Set&) const;
    void			getDataPointSet( const SectionID&,
						  DataPointSet&,
						  float shift=0.0) const;
    void			fillBinIDValueSet(const SectionID&,
						 BinIDValueSet&,
						 Pos::Provider3D* prov=0) const;

    EMObjectIterator*		createIterator(const EM::SectionID&,
					       const TrcKeyZSampling* =0) const;
protected:

    Geometry::BinIDSurface*	createSectionGeometry() const;

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

    virtual float		getZ(const TrcKey&) const;
				//!< Fast: reads from the first section
    virtual bool		setZ(const TrcKey&,float z,bool addtohist);
				//!< Fast: writes to the first section
    virtual bool		setZAndNodeSourceType(const TrcKey&,float z,
				    bool addtohist, NodeSourceType type=Auto);

    virtual void		setNodeSourceType(const TrcKey&,NodeSourceType);
    virtual void		setNodeSourceType(const PosID&,NodeSourceType);
    virtual bool		isNodeSourceType(const PosID&,
						 NodeSourceType) const;
    virtual bool		isNodeSourceType(const TrcKey&,
						 NodeSourceType) const;
    virtual bool		isNodeLocked(const PosID&)const;

    bool			hasNodeSourceType(const PosID&) const;

    virtual bool		hasZ(const TrcKey&) const;
				//!< Fast: checks only the first section
    virtual Coord3		getCoord(const TrcKey&) const;
    virtual void		setAttrib(const TrcKey&,int attr,int yn,
					  bool addtohist);
    virtual bool		isAttrib(const TrcKey&,int attr) const;

    virtual float		getZValue(const Coord&,bool allow_udf=true,
					  int nr=0) const;
				//!< Slow: if you need the choices

    TrcKeySampling		range() const;
    Interval<float>		getZRange() const;

    void			removeAll();
    Horizon3DGeometry&		geometry();
    const Horizon3DGeometry&	geometry() const;

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

    uiString			getUserTypeStr() const { return userTypeStr(); }
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
    void			setNodeLocked(const TrcKey&,bool locked);
    bool			isNodeLocked(const TrcKey&) const;
    void			lockAll();
    void			unlockAll();
    const Array2D<char>*	getLockedNodes() const;

    void			setParentColor(const OD::Color&);
    const OD::Color&		getParentColor() const;

    virtual bool		setPos(const EM::PosID&,const Coord3&,
				       bool addtohistory);
    virtual bool		setPos(const EM::SectionID&,const EM::SubID&,
				       const Coord3&,bool addtohistory);
    virtual void		apply(const Pos::Filter&);

protected:
    enum			ArrayType{Parents,Children,LockNode,NodeSource};

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    void			fillDisplayPar(IOPar&) const;
    bool			useDisplayPar(const IOPar&);

    const IOObjContext&		getIOObjContext() const;
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
    OD::GeomSystem		getSurveyID() const
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
    int			nextStep();

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

