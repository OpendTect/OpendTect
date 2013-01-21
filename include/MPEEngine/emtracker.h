#ifndef emtracker_h
#define emtracker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "factory.h"
#include "sets.h"
#include "emposid.h"
#include "cubesampling.h"
#include "refcount.h"

class Executor;

namespace Geometry { class Element; };
namespace EM { class EMObject; };
namespace Attrib { class SelSpec; }

namespace MPE
{

class SectionTracker;
class TrackPlane;
class EMSeedPicker;

/*!
\brief Tracks EM objects.
*/

mExpClass(MPEEngine) EMTracker
{
mRefCountImplWithDestructor(EMTracker,virtual ~EMTracker(),delete this;);
public:
    				EMTracker( EM::EMObject* );

    BufferString		objectName() const;
    EM::ObjectID		objectID() const;

    virtual bool		is2D() const		{ return false; }

    void			setTypeStr(const char* type)
				{ type_ = type; }
    const char*			getTypeStr() const	{ return type_; }

    virtual bool		isEnabled() const	{ return isenabled_; }
    virtual void		enable(bool yn)		{ isenabled_=yn; }

    virtual bool		trackSections(const TrackPlane&);
    virtual bool		trackIntersections(const TrackPlane&);
    virtual Executor*		trackInVolume();
    virtual bool		snapPositions( const TypeSet<EM::PosID>& );

    virtual CubeSampling	getAttribCube( const Attrib::SelSpec& ) const;
    void			getNeededAttribs(
				    ObjectSet<const Attrib::SelSpec>&) const;

    SectionTracker*		getSectionTracker(EM::SectionID,
	    					  bool create=false);
    virtual EMSeedPicker*	getSeedPicker(bool createifnotpresent=true)
				{ return 0; }
    void 			applySetupAsDefault(const EM::SectionID);

    const char*			errMsg() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    virtual SectionTracker*	createSectionTracker(EM::SectionID) = 0;
    virtual void		erasePositions(EM::SectionID,
	    				       const TypeSet<EM::SubID>&);

    bool			isenabled_;
    ObjectSet<SectionTracker>	sectiontrackers_;
    BufferString		errmsg_;
    const char*			type_;

    EM::EMObject*		emObject()      { return emobject_; }
    void			setEMObject(EM::EMObject*);

    static const char*		setupidStr()	{ return "SetupID"; }
    static const char*		sectionidStr()	{ return "SectionID"; }

private:
    EM::EMObject*		emobject_;
};


mDefineFactory1Param( MPEEngine, EMTracker, EM::EMObject*, TrackerFactory );


}; // namespace MPE

#endif


