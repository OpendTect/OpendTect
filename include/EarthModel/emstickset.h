#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "binidvalue.h"
#include "emposid.h"
#include "emobject.h"
#include "coord.h"


namespace EM
{

typedef SectionID StickID;
typedef SubID KnotID;

class EMManager;

/*!
\brief Stick set
*/

mExpClass(EarthModel) StickSet : public EMObject
{
public:
    static const char*	typeStr();
    static EMObject*	create( EMManager& );
    static void		initClass(EMManager&);

    const char*		getTypeStr() const { return typeStr(); }

    int			nrSections() const { return nrSticks(); }
    EM::SectionID	sectionID(int idx) const { return stickID(idx); }

    int			nrSticks() const;
    StickID		stickID(int idx) const;
    StickID		addStick( bool addtohistory );
    void		removeStick(const StickID&);

    int			nrKnots(const StickID&) const;
    KnotID		firstKnot(const StickID&) const;

    bool		setPos( const StickID&, const KnotID&,
	    			const Coord3&, bool addtohistory );
    bool		setPos(const EM::PosID&, const Coord3&, bool addtohist);

    Coord3		getPos(const EM::PosID&) const;
    Coord3		getPos(const StickID&, const KnotID&) const;

    bool		isLoaded() const;
    Executor*		saver();
    Executor*		loader();

protected:
    int			getStickIndex(const StickID&) const;

    friend class		EMManager;
    friend class		EMObject;

    				StickSet(EMManager&);
    				~StickSet();
    void				cleanUp();
    virtual const IOObjContext&		getIOObjContext() const;

    ObjectSet<TypeSet<BinIDValue> >	sticks;
    TypeSet<StickID>			stickids;
    TypeSet<KnotID>			firstknots;
    bool				isloaded;
};

} // namespace EM
