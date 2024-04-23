#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"

#include "color.h"
#include "emposid.h"
#include "enums.h"
#include "multidimstorage.h"
#include "multiid.h"
#include "ranges.h"
#include "rowcol.h"
#include "sharedobject.h"

class BinIDValueSet;
class IOObj;
class IOObjContext;
class Undo;
class UnitOfMeasure;
template <class T> class Smoother1D;

namespace EM { class Horizon3D; }
namespace ZDomain { class Info; }

namespace Vel
{

class PicksMgr;

mExpClass(Velocity) Pick
{
public:
			Pick(float depth=mUdf(float),
			     float vel=mUdf(float),
			     float offset=mUdf(float),
			     EM::ObjectID=EM::ObjectID::udf());
    virtual		~Pick();

    bool		operator==(const Pick& b) const;

    float		depth_;
    float		offset_;
    float		vel_;
    EM::ObjectID	emobjid_;
};


/*!Holds picks that the user has done, typically in a semblance plot. */

mExpClass(Velocity) Picks : public SharedObject
{ mODTextTranslationClass(Picks)
public:
			Picks(const ZDomain::Info* =nullptr);

    enum PickType	{ RMO, RMS, Delta, Epsilon, Eta };
			mDeclareEnumUtils(PickType)
    PickType		pickType() const;
    void		setPickType(PickType,bool resetcolor);

    bool		setColor(const OD::Color&,bool savedefault);
			//!<\returns false if savedefault failed.
    const OD::Color&	getColor() const { return color_; }
    bool		getDefaultColor(OD::Color&) const;

    Undo&		undo();

    void		removeAll(bool undo=true,bool interactionend=true);
    bool		isEmpty() const;

    void			setSnappingInterval(const StepInterval<float>&);
    const StepInterval<float>&	getSnappingInterval() const { return snapper_; }
    RowCol			find(const BinID&,const Pick&) const;
    RowCol			set(const BinID&, const Pick&,
				    bool undo=true,bool interactionend=true);
    void			set(const RowCol&,
				    const Pick&,bool undo=true,
				    bool interactionend=true);
    int				get(const BinID&, TypeSet<float>* depths,
				    TypeSet<float>* vals,
				    TypeSet<RowCol>*,
				    TypeSet<EM::ObjectID>*,
				    bool interpolatehors ) const;
				//!<\returns number of picks
    void			get(const BinID&,TypeSet<Pick>&,
				    bool interpolatehors,
				    bool normalizerefoffset ) const;
				//!<\returns number of picks
    bool			get(const RowCol&,BinID&,Pick& );
    void			get(const EM::ObjectID&,TypeSet<RowCol>&) const;
    void			remove(const RowCol&,bool undo=true,
				       bool interactionend=true);

    const MultiDimStorage<Pick>& getAll() const { return picks_; }

    CNotifier<Picks,const BinID&> change;
    CNotifier<Picks,const BinID&> changelate;
				/*!<Triggers after pickchange. */

    bool			isChanged() const;
    void			resetChangedFlag() { changed_ = false; }

    bool			store(const IOObj*);
				/*!< ioobj is not transferred */

    const MultiID&		storageID() const;

    const ZDomain::Info&	zDomain() const;
    bool			zIsTime() const;
    bool			zInMeter() const;
    bool			zInFeet() const;
    const UnitOfMeasure*	zUnit() const;
    const UnitOfMeasure*	velUnit() const;
				//!<Only for RMS type

    Smoother1D<float>*		getSmoother()		{ return smoother_; }
    const Smoother1D<float>*	getSmoother() const	{ return smoother_; }
    void			setSmoother(Smoother1D<float>*);
				//!<Becomes mine

    uiString			errMsg() const;

    void			setAll(float vel,bool undo=true);

    static const IOObjContext&	getStorageContext();
    static void			setContextPickType(IOObjContext&,PickType);

    float			refOffset() const	{ return refoffset_; }
    void			setReferenceOffset(double n);

    const MultiID&		gatherID() const;
    void			setGatherID(const MultiID&);


    void			addHorizon(const MultiID&,
					   bool addzeroonfail=false);
    void			addHorizon(EM::Horizon3D*);
    int				nrHorizons() const;

    EM::ObjectID		getHorizonID(int) const;
    void			removeHorizon(EM::ObjectID);
    EM::Horizon3D*		getHorizon(EM::ObjectID);
    const EM::Horizon3D*	getHorizon(EM::ObjectID) const;
    bool			interpolateVelocity(EM::ObjectID,
				    float searchradius,BinIDValueSet&) const;
				/*!<Interpolates vel at all locations in
				    the valset. First value in valset will
				    be horizon depth, second will be velocity.*/
    char			getHorizonStatus(const BinID&) const;
				/*!<\retval 0 no defined hors.
				    \retval 1 some defined, some undefined.
				    \retval 2 all defined. */


    static const char*		sKeyVelocityPicks();
    static const char*		sKeyRefOffset();
    static const char*		sKeyGatherID();
    static const char*		sKeyNrHorizons();
    static const char*		sKeyHorizonPrefix();
    static const char*		sKeyPickType();

protected:
    virtual			~Picks();

    void			getColorKey(BufferString&) const;
    void			removeHorizons();
    friend			class PicksMgr;
    void			fillIOObjPar(IOPar&) const;
    Picks&			setZDomain(const ZDomain::Info&);
    bool			useIOObjPar(const IOPar&);
    double			normalizeRMO(float depth,float rmo,
					     float offset) const;
				/*!<Given an rmo at a certain offset,
				    what is the rmo at refoffset_. */

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			horizonChangeCB(CallBacker*);

    double			refoffset_ = 0.;
    MultiID			gatherid_;
    bool			load(const IOObj*);
    StepInterval<float>		snapper_;
    MultiID			storageid_;
    MultiDimStorage<Pick>	picks_;

    Undo*			undo_ = nullptr;

    uiString			errmsg_;
    Smoother1D<float>*		smoother_ = nullptr;
    ObjectSet<EM::Horizon3D>	horizons_;

    PickType			picktype_;
    OD::Color			color_;

    bool			changed_ = false;
    const ZDomain::Info*	zdomaininfo_;

};


mExpClass(Velocity) PicksMgr : public CallBacker
{
public:
				PicksMgr();
				~PicksMgr();

    Picks*			get(const MultiID&,bool gathermid,
				    bool create, bool forcefromstorage );

    const char*			errMsg() const;
protected:
    friend			class Picks;

    void			surveyChange(CallBacker*);

    BufferString		errmsg_;
    ObjectSet<Picks>		velpicks_;
};


mGlobal(Velocity) PicksMgr& VPM();

} // namespace Vel
