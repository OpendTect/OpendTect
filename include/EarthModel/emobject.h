#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.53 2005-10-18 18:42:19 cvskris Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "callback.h"
#include "emposid.h"
#include "multiid.h"
#include "position.h"
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


class EMObjectIterator
{
public:
    virtual		~EMObjectIterator() {}
    virtual EM::PosID	next() 		= 0;
    			/*!<posid.objectID()==-1 when there are no more pids*/
    virtual int		aproximateSize() const { return -1; }
    virtual int		maximumSize() const { return -1; }
};



/*!\brief Earth Model Object */

class EMObject : public CallBacker
{ mRefCountImpl( EMObject );    
public:
    				EMObject( EMManager& );
    const ObjectID&		id() const { return id_; }
    virtual const char*		getTypeStr() const			= 0;
    const MultiID&		multiID() const { return storageid; }
    void			setMultiID( const MultiID& mid );

    BufferString		name() const;

    virtual int			nrSections() const 			= 0;
    virtual SectionID		sectionID( int ) const			= 0;
    virtual BufferString	sectionName( const SectionID& ) const;
    virtual bool		canSetSectionName() const;
    virtual bool		setSectionName( const SectionID&, const char*,
	    					bool addtohistory );
    virtual int			sectionIndex( const SectionID& ) const;
    virtual bool		removeSection( SectionID, bool hist )
    					{ return false; }

    const Geometry::Element*	getElement( SectionID ) const;

    const Color&		preferredColor() const;
    void			setPreferredColor(const Color&);

    virtual Coord3		getPos(const EM::PosID&) const;
    virtual Coord3		getPos(const EM::SectionID&,
	    			       const EM::SubID& ) const;
    virtual bool		isDefined( const EM::PosID& ) const;
    virtual bool		setPos(const EM::PosID&,
	    			       const Coord3&,
				       bool addtohistory);
    virtual bool		setPos(const EM::SectionID&,
	    			       const EM::SubID&,
	    			       const Coord3&,
				       bool addtohistory);
    virtual bool		unSetPos(const EM::PosID&, bool addtohistory );
    virtual bool		isAtEdge( const EM::PosID& ) const;

    void			changePosID( const EM::PosID& from, 
	    				     const EM::PosID& to,
					     bool addtohistory );
    				/*!<Tells the object that the node former known
				    as from is now called to. Function will also
				    exchange set the positiono of to to the 
				    posion of from. */

    virtual void		getLinkedPos( const EM::PosID& posid,
					  TypeSet<EM::PosID>& ) const
    					{ return; }
    				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual EMObjectIterator*	createIterator( const EM::SectionID& ) const
				{ return 0; }
    				/*!< creates an interator. If the sectionid is
				     -1, all sections will be traversed. */

    virtual int			nrPosAttribs() const;
    virtual int			posAttrib(int idx) const;
    virtual void		removePosAttrib( int attr,
	    					 bool addtohistory=true );
    virtual void		setPosAttrib( const EM::PosID&,
				    int attr, bool yn, bool addtohistory=true );
    				//!<Sets/unsets the posattrib depending on yn.
    virtual bool		isPosAttrib(const EM::PosID&, int attr) const;
    virtual const char*		posAttribName(int) const;
    virtual int			addPosAttribName(const char*);
    const TypeSet<PosID>*	getPosAttribList(int) const;

    CNotifier<EMObject,const EMObjectCallbackData&>	notifier;

    virtual Executor*		loader() { return 0; }
    virtual bool		isLoaded() const {return false;}
    virtual Executor*		saver() { return 0; }
    virtual bool		isChanged( int what=-1 ) const
    				{ return changed; }
    				/*!<\param what	Says what the query is about.
						-1 mean any change, all other
						figures are dependent on impl.
				*/
    virtual void		resetChangedFlag(int what=-1) { changed=false; }

    const char*			errMsg() const
    				{ return errmsg[0]
				    ? (const char*) errmsg : (const char*) 0; }


    virtual bool		usePar( const IOPar& );
    virtual void		fillPar( IOPar& ) const;

    static int			sPermanentControlNode;
    static int			sTemporaryControlNode;
    static int			sEdgeControlNode;
    static int			sTerminationNode;
    static int			sSeedNode;

protected:
    virtual Geometry::Element*	getElementInternal( SectionID ) { return 0; }

    void			posIDChangeCB(CallBacker*);
    virtual const IOObjContext&	getIOObjContext() const = 0;
    ObjectID			id_;
    MultiID			storageid;
    class EMManager&		manager;
    BufferString		errmsg;

    Color&			preferredcolor;

    ObjectSet<TypeSet<PosID> >	posattribs;
    TypeSet<int>		attribs;

    bool			changed;

    static const char*		prefcolorstr;
    static const char*		nrposattrstr;
    static const char*		posattrprefixstr;
    static const char*		posattrsectionstr;
    static const char*		posattrposidstr;
};


typedef EMObject*(*EMObjectCreationFunc)(EMManager&);

class ObjectFactory
{
public:
    				ObjectFactory( EMObjectCreationFunc,
				       	       const IOObjContext&,
				       	       const char* );
    EMObject*			loadObject( const MultiID& mid ) const;
    EMObject*			createObject( const char* name,
	    				      bool tmpobj ) const;
    				/*!<\Creates a new object. If tmobj is false,
				     a IOObj entry will be created for this
				     object.*/
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
