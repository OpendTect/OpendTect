#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.62 2006-11-06 10:33:16 cvsjaap Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "callback.h"
#include "emposid.h"
#include "multiid.h"
#include "position.h"
#include "refcount.h"
#include "draw.h"

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


/*! Iterator that iterates a number of positions (normally all) on an EMObject.
The object is created by EMObject::createIterator, and the next() function is
called until no more positions can be found. */


class EMObjectIterator
{
public:
    virtual		~EMObjectIterator() {}
    virtual EM::PosID	next() 		= 0;
    			/*!<posid.objectID()==-1 when there are no more pids*/
    virtual int		aproximateSize() const { return -1; }
    virtual int		maximumSize() const { return -1; }
};


class PosAttrib
{
public:
    			PosAttrib()
			    : style_(MarkerStyle3D::Cube,5,Color::White) {}

    enum Type		{ PermanentControlNode, TemporaryControlNode,
			  EdgeControlNode, TerminationNode, SeedNode };

    Type		type_;
    TypeSet<PosID>	posids_;

    MarkerStyle3D	style_;

    bool		locked_;    
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
    virtual SectionID		sectionID(int) const			= 0;
    virtual BufferString	sectionName(const SectionID&) const;
    virtual bool		canSetSectionName() const;
    virtual bool		setSectionName(const SectionID&,const char*,
	    				       bool addtohistory);
    virtual int			sectionIndex( const SectionID&) const;
    virtual bool		removeSection( SectionID, bool hist )
    					{ return false; }

    const Geometry::Element*	sectionGeometry(const SectionID&) const;
    Geometry::Element*		sectionGeometry(const SectionID&);

    const Color&		preferredColor() const;
    void			setPreferredColor(const Color&);

    virtual Coord3		getPos(const EM::PosID&) const;
    virtual Coord3		getPos(const EM::SectionID&,
	    			       const EM::SubID&) const;
    virtual bool		isDefined(const EM::PosID&) const;
    virtual bool		isDefined(const EM::SectionID&,
					  const EM::SubID&) const;
    virtual bool		setPos(const EM::PosID&,const Coord3&,
				       bool addtohistory);
    virtual bool		setPos(const EM::SectionID&,const EM::SubID&,
	    			       const Coord3&,bool addtohistory);
    virtual bool		unSetPos(const EM::PosID&,bool addtohistory);
    virtual bool		unSetPos(const EM::SectionID&,const EM::SubID&,
					 bool addtohistory);


    virtual bool		enableGeometryChecks(bool);
    virtual bool		isGeometryChecksEnabled() const;

    virtual bool		isAtEdge(const EM::PosID&) const;

    void			changePosID(const EM::PosID& from, 
	    				    const EM::PosID& to,
					    bool addtohistory);
    				/*!<Tells the object that the node former known
				    as from is now called to. Function will also
				    exchange set the position of to to the 
				    posion of from. */

    virtual void		getLinkedPos(const EM::PosID& posid,
					  TypeSet<EM::PosID>&) const
    					{ return; }
    				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual EMObjectIterator*	createIterator(const EM::SectionID&, 
	    				       const CubeSampling* =0) const
				{ return 0; }
    				/*!< creates an iterator. If the sectionid is
				     -1, all sections will be traversed. */

    virtual int			nrPosAttribs() const;
    virtual int			posAttrib(int idx) const;
    virtual void		addPosAttrib(int attr);
    virtual void		removePosAttribList(int attr,
						    bool addtohistory=true);
    virtual void		setPosAttrib(const EM::PosID&,
				    int attr,bool yn,bool addtohistory=true);
    				//!<Sets/unsets the posattrib depending on yn.
    virtual bool		isPosAttrib(const EM::PosID&,int attr) const;
    virtual const char*		posAttribName(int) const;
    virtual int			addPosAttribName(const char*);
    const TypeSet<PosID>*	getPosAttribList(int attr) const;
    const MarkerStyle3D&	getPosAttrMarkerStyle(int attr);
    void			setPosAttrMarkerStyle(int attr, 
						      const MarkerStyle3D&);
    virtual void		lockPosAttrib(int attr,bool yn);
    virtual bool		isPosAttribLocked(int attr) const;

    CNotifier<EMObject,const EMObjectCallbackData&>	notifier;

    virtual Executor*		loader()		{ return 0; }
    virtual bool		isLoaded() const	{ return false; }
    virtual Executor*		saver()			{ return 0; }
    virtual bool		isChanged() const	{ return changed; }
    virtual bool		isEmpty() const;
    virtual void		resetChangedFlag()	{ changed=false; }
    bool			isFullyLoaded() const	{ return fullyloaded; }
    void			setFullyLoaded(bool yn) { fullyloaded=yn; }

    virtual bool		isLocked() const	{ return locked; }
    virtual void		lock(bool yn)		{ locked=yn;}

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
    virtual Geometry::Element*	sectionGeometryInternal(const SectionID&);

    void			posIDChangeCB(CallBacker*);
    virtual const IOObjContext&	getIOObjContext() const = 0;
    ObjectID			id_;
    MultiID			storageid;
    class EMManager&		manager;
    BufferString		errmsg;


    Color&			preferredcolor;

    ObjectSet<PosAttrib>	posattribs;
    TypeSet<int>		attribs;

    bool			changed;
    bool			fullyloaded;
    bool			locked;

    static const char*		prefcolorstr;
    static const char*		nrposattrstr;
    static const char*		posattrprefixstr;
    static const char*		posattrsectionstr;
    static const char*		posattrposidstr;
    
    static const char*		markerstylestr;
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
