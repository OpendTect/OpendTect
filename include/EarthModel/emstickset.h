#ifndef emstickset_h
#define emstickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emstickset.h,v 1.2 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "sets.h"
#include "position.h"

/*!
*/

class BinID;
class RowCol;
class CubeSampling;
template <class T> class Interval;
template <class T> class StepInterval;


namespace EM
{

typedef PatchID StickID;
typedef SubID KnotID;

class EMManager;

class StickSet : public EMObject
{
public:
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
    
    friend class			EMManager;
    friend class			EMObject;

    					StickSet(EMManager&, const MultiID& );
    					~StickSet();
    void				cleanUp();

    ObjectSet<TypeSet<BinIDValue> >	sticks;
    TypeSet<StickID>			stickids;
    TypeSet<KnotID>			firstknots;
    bool				isloaded;
};


}; // Namespace


#endif
