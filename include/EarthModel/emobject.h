#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.23 2004-05-12 08:38:34 kristofer Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "callback.h"
#include "emposid.h"
#include "multiid.h"

class IOObj;
class Executor;
struct CubeSampling;
class IOObjContext;
class Color;

namespace EM
{
class EMManager;

/*!\brief Earth Model Object */

class EMObject : public CallBacker
{
public:
    static EMObject*		create(const IOObj&,EMManager&);

    void			ref() const;
    void			unRef() const;
    void			unRefNoDel() const;

    				EMObject( EMManager&, const EM::ObjectID&);
    virtual			~EMObject( );
    const ObjectID&		id() const { return id_; }
    MultiID			multiID() const;

    BufferString		name() const;

    const Color&		preferredColor() const;
    void			setPreferredColor(const Color&);
    Notifier<EMObject>		prefColorChange;

    virtual Coord3		getPos(const EM::PosID&) const = 0;
    virtual bool		setPos(const EM::PosID&,
	    			       const Coord3&,
				       bool addtohistory ) = 0;

    virtual void		getLinkedPos( const EM::PosID& posid,
					  TypeSet<EM::PosID>& ) const
    					{ return; }
    				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual void		setPosAttrib( const EM::PosID&,
	    					int attr, bool yn );
    virtual bool		isPosAttrib(const EM::PosID&, int attr) const;
    virtual const char*		posAttribName(int) const;
    virtual int			nrPosAttribs() const;
    virtual int			addPosAttribName(const char*);
    CNotifier<EMObject, PosID>*	getPosAttribChNotifier( int, bool create );

    CNotifier<EMObject, PosID>	poschnotifier;

    virtual Executor*		loader() { return 0; }
    virtual bool		isLoaded() const {return false;}
    virtual Executor*		saver() { return 0; }
    virtual bool		isChanged( int what=-1 ) const { return false; }
    				/*!<\param what	Says what the query is about.
						-1 mean any change, all other
						figures are dependent on impl.
				*/
				

    const char*			errMsg() const
    				{ return errmsg[0]
				    ? (const char*) errmsg : (const char*) 0; }

    static int			sPermanentControlNode;
    static int			sTemporaryControlNode;
    static int			sEdgeControlNode;

protected:
    virtual const IOObjContext&	getIOObjContext() const = 0;
    ObjectID			id_;
    class EMManager&		manager;
    BufferString		errmsg;

    Color&			preferredcolor;

    ObjectSet<TypeSet<PosID> >	posattribs;
    TypeSet<int>		attribs;
    ObjectSet<CNotifier<EMObject, PosID> >	posattrchnotifiers;
};

}; // Namespace

/*!\mainpage Earth Model objects

  Objects like horizons, well tracks and bodies are all earth model objects.
  Such objects can be described in various ways, and the EM way is the
  way we have chosen for OpendTect.

  A big part of this module deals with surfaces of some kind. Horizons, faults,
  and sets of (fault-)sticks each have their own peculiarities.

  Most interesting classes:

  - EM::EMObject, base class for the EarthModel objects
  - EM::EMManager, responsible for the loading and deleting of the objects

  Earth models have the nasty habit of changing over time. Therefore, edit
  history matters are handled by the EM::History objects.

*/

#endif
