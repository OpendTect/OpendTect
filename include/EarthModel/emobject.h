#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.36 2005-02-16 14:13:20 cvskris Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "callback.h"
#include "emposid.h"
#include "multiid.h"
#include "refcount.h"

class IOObj;
class Executor;
struct CubeSampling;
class IOObjContext;
class Color;

namespace Geometry { class Element; }

namespace EM
{
class EMManager;

class EMObjectCallbackData
{
public:
    		EMObjectCallbackData() 
		    : pid0( 0, 0, 0 )
		    , pid1( 0, 0, 0 )
		    , attrib( -1 )
		    , event( EMObjectCallbackData::Undef )
		{}

    enum Event { Undef, PositionChange, PosIDChange, PrefColorChange, Removal,
   		 AttribChange, SectionChange }	event;

    EM::PosID	pid0;
    EM::PosID	pid1;	//Only used in PosIDChange
    int		attrib; //Used only with AttribChange
};


/*!\brief Earth Model Object */

class EMObject : public CallBacker
{ mRefCountImpl( EMObject );    
public:
    				EMObject( EMManager&, const EM::ObjectID&);
    const ObjectID&		id() const { return id_; }
    void			setID( const EM::ObjectID& nid ) { id_ = nid; }
    virtual const char*		getTypeStr() const			= 0;
    MultiID			multiID() const;

    BufferString		name() const;

    virtual int			nrSections() const 			= 0;
    virtual SectionID		sectionID( int ) const			= 0;
    virtual bool		removeSection( SectionID, bool hist )
    					{ return false; }

    virtual const Geometry::Element*	getElement( SectionID ) const;

    const Color&		preferredColor() const;
    void			setPreferredColor(const Color&);

    virtual Coord3		getPos(const EM::PosID&) const;
    virtual bool		isDefined( const EM::PosID& ) const;
    virtual bool		setPos(const EM::PosID&,
	    			       const Coord3&,
				       bool addtohistory );
    virtual bool		unSetPos(const EM::PosID&, bool addtohistory );

    void			changePosID( const EM::PosID& from, 
	    				     const EM::PosID& to,
					     bool addtohistory );

    virtual void		getLinkedPos( const EM::PosID& posid,
					  TypeSet<EM::PosID>& ) const
    					{ return; }
    				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual int			nrPosAttribs() const;
    virtual int			posAttrib(int idx) const;
    virtual void		removePosAttrib(int attr);
    virtual void		setPosAttrib( const EM::PosID&,
	    					int attr, bool yn );
    virtual bool		isPosAttrib(const EM::PosID&, int attr) const;
    virtual const char*		posAttribName(int) const;
    virtual int			addPosAttribName(const char*);
    const TypeSet<PosID>*	getPosAttribList(int) const;

    CNotifier<EMObject,const EMObjectCallbackData&>	notifier;

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


    virtual bool		usePar( const IOPar& );
    virtual void		fillPar( IOPar& ) const;

    static int			sPermanentControlNode;
    static int			sTemporaryControlNode;
    static int			sEdgeControlNode;
    static int			sTerminationNode;

protected:
    virtual Geometry::Element*	getElement( SectionID ) { return 0; }

    void			posIDChangeCB(CallBacker*);
    virtual const IOObjContext&	getIOObjContext() const = 0;
    ObjectID			id_;
    class EMManager&		manager;
    BufferString		errmsg;

    Color&			preferredcolor;

    ObjectSet<TypeSet<PosID> >	posattribs;
    TypeSet<int>		attribs;

    static const char*		prefcolorstr;
    static const char*		nrposattrstr;
    static const char*		posattrprefixstr;
    static const char*		posattrsectionstr;
    static const char*		posattrposidstr;
};


typedef EMObject*(*EMObjectCreationFunc)(const ObjectID&, EMManager&);

class ObjectFactory
{
public:
    				ObjectFactory( EMObjectCreationFunc,
				       	       const IOObjContext&,
				       	       const char* );
    EMObject*			create( const char* name, bool tmpobj,
	    				EMManager& );
    const char*			typeStr() const { return typestr; }
    const IOObjContext&		ioContext() const { return context; }
protected:
    EMObjectCreationFunc	creationfunc;
    const IOObjContext&		context;
    const char*			typestr;
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
