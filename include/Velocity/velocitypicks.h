#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "multidimstorage.h"
#include "notify.h"
#include "color.h"
#include "enums.h"
#include "dbkey.h"
#include "ranges.h"
#include "rowcol.h"

class Undo;
class IOObj;
template <class T> class Smoother1D;
class BinnedValueSet;
class IOObjContext;

namespace EM { class Horizon3D; }

namespace Vel
{

class PicksMgr;

mExpClass(Velocity) Pick
{
public:
			Pick(float depth=mUdf(float),
			     float vel=mUdf(float),
			     float offset=mUdf(float),
			     DBKey=DBKey::getInvalid());
    bool		operator==(const Pick& b) const;

    float		depth_;
    float		offset_;
    float		vel_;
    DBKey		emobjid_;
};


/*!Holds picks that the user has done, typically in a semblance plot. */

mExpClass(Velocity) Picks : public RefCount::Referenced
			  , public CallBacker
{
public:
			Picks();
			Picks(bool zit);

    enum PickType	{ RMO, RMS, Delta, Epsilon, Eta };
			mDeclareEnumUtils(PickType)
    PickType		pickType() const;
    void		setPickType( PickType, bool resetcolor );
    const char*		zDomain() const;

    bool		setColor(const Color&,bool savedefault);
			//!<\returns false if savedefault failed.
    const Color&	getColor() const { return color_; }
    bool		getDefaultColor(Color&) const;


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
				    DBKeySet*,
				    bool interpolatehors ) const;
				//!<\returns number of picks
    void			get(const BinID&, TypeSet<Pick>&,
				    bool interpolatehors,
				    bool normalizerefoffset ) const;
				//!<\returns number of picks
    bool			get(const RowCol&, BinID&, Pick& );
    void			get(const DBKey&,TypeSet<RowCol>&) const;
    void			remove(const RowCol&,
				       bool undo=true,bool interactionend=true);

    const MultiDimStorage<Pick>& getAll() const { return picks_; }

    CNotifier<Picks,const BinID&> change;
    CNotifier<Picks,const BinID&> changelate;
				/*!<Triggers after pickchange. */

    bool			isChanged() const;
    void			resetChangedFlag() { changed_ = false; }

    bool			store(const IOObj*);
				/*!< ioobj is not transferred */

    const DBKey&		storageID() const;
    bool			zIsTime() const		{ return zit_; }

    Smoother1D<float>*		getSmoother()		{ return smoother_; }
    const Smoother1D<float>*	getSmoother() const	{ return smoother_; }
    void			setSmoother(Smoother1D<float>*);
				//!<Becomes mine

    const uiString		errMsg() const;
    static const char*		sKeyIsTime();

    void			setAll(float vel,bool undo=true);

    static const IOObjContext&	getStorageContext();
    static void			setContextPickType(IOObjContext&,PickType);

    float			refOffset() const	{ return refoffset_; }
    void			setReferenceOffset(float n);

    const DBKey&		gatherID() const;
    void			setGatherID(const DBKey&);


    void			addHorizon(const DBKey&,
					   bool addzeroonfail=false);
    void			addHorizon(EM::Horizon3D*);
    int				nrHorizons() const;

    DBKey			getHorizonID(int) const;
    void			removeHorizon(const DBKey&);
    EM::Horizon3D*		getHorizon(const DBKey&);
    const EM::Horizon3D*	getHorizon(const DBKey&) const;
    bool			interpolateVelocity(const DBKey&,
				    float searchradius,BinnedValueSet&) const;
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
				~Picks();
    void			getColorKey(BufferString&) const;
    void			removeHorizons();
    friend			class PicksMgr;
    void			fillIOObjPar(IOPar&) const;
    bool			useIOObjPar(const IOPar&);
    float			normalizeRMO(float depth,float rmo,
					     float offset) const;
				/*!<Given an rmo at a certain offset,
				    what is the rmo at refoffset_. */

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			horizonChangeCB(CallBacker*);

    float			refoffset_;
    DBKey			gatherid_;
    bool			load(const IOObj*);
    StepInterval<float>		snapper_;
    DBKey			storageid_;
    MultiDimStorage<Pick>	picks_;

    Undo*			undo_;

    uiString			errmsg_;
    Smoother1D<float>*		smoother_;
    ObjectSet<EM::Horizon3D>	horizons_;

    PickType			picktype_;
    Color			color_;

    bool			changed_;
    bool			zit_;

};


mExpClass(Velocity) PicksMgr : public CallBacker
{
public:
				PicksMgr();
				~PicksMgr();

    Picks*			get(const DBKey&,bool gathermid,
				    bool create, bool forcefromstorage );

    const uiString		errMsg() const;
protected:
    friend			class Picks;

    void			surveyChange(CallBacker*);

    uiString			errmsg_;
    ObjectSet<Picks>		velpicks_;
};


mGlobal(Velocity) PicksMgr& VPM();

} // namespace Vel
