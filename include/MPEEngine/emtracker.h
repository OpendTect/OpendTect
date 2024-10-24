#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emobject.h"

class Executor;

namespace Geometry { class Element; }
namespace Attrib { class SelSpec; }

namespace MPE
{

class EMSeedPicker;
class SectionTracker;

/*!
\brief Tracks EM objects.
*/

mExpClass(MPEEngine) EMTracker : public ReferencedObject
			       , public CallBacker
{
mODTextTranslationClass(EMTracker)
public:

    ConstRefMan<EM::EMObject>	emObject() const;
    RefMan<EM::EMObject>	emObject();
    BufferString		objectName() const;
    EM::ObjectID		objectID() const;

    virtual bool		is2D() const				= 0;

    void			setTypeStr( const char* tp )
				{ type_ = tp; }
    const char*			getTypeStr() const	{ return type_.buf(); }

    virtual bool		isEnabled() const	{ return isenabled_; }
    virtual void		enable(bool yn)		{ isenabled_=yn; }

    virtual bool		snapPositions(const TypeSet<TrcKey>&);

    virtual TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;
    void			getNeededAttribs(
					TypeSet<Attrib::SelSpec>&) const;

    virtual bool		hasTrackingMgr() const	   { return false; }
    virtual bool		createMgr()		   { return false; }
    virtual void		startFromSeeds(const TypeSet<TrcKey>&)	  {}
    virtual void		initTrackingMgr()			  {}
    virtual bool		trackingInProgress() const { return false; }
    virtual void		stopTracking()				  {}
    virtual void		updateFlatCubesContainer(const TrcKeyZSampling&,
							 bool addremove)  {}
				/*!< add = true, remove = false. */

    SectionTracker*		cloneSectionTracker();
    SectionTracker*		getSectionTracker(bool create=false);
    virtual EMSeedPicker*	getSeedPicker( bool createifnotpresent=true )
				{ return nullptr; }
    void			applySetupAsDefault();

    const char*			errMsg() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    Notifier<EMTracker>		trackingFinished;

protected:
				EMTracker(EM::EMObject&);
    virtual			~EMTracker();

    void			trackingFinishedCB(CallBacker*);

    bool			isenabled_		= true;
    ObjectSet<SectionTracker>	sectiontrackers_;
    BufferString		errmsg_;
    BufferString		type_;

    static const char*		setupidStr()	{ return "SetupID"; }
    static const char*		sectionidStr()	{ return "SectionID"; }

private:

    virtual SectionTracker*	createSectionTracker()			= 0;

    WeakPtr<EM::EMObject>	emobject_;

public:
// Deprecated public functions
    mDeprecated("Use without SectionID")
    SectionTracker*		createSectionTracker(EM::SectionID)
				{ return createSectionTracker(); }
    mDeprecated("Use without SectionID")
    SectionTracker*		getSectionTracker(EM::SectionID,
						  bool create=false)
				{ return getSectionTracker(create); }
    mDeprecated("Use without SectionID")
    void			applySetupAsDefault(const EM::SectionID)
				{ applySetupAsDefault(); }

};

} // namespace MPE
