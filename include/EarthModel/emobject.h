#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.98 2012-08-03 13:00:19 cvskris Exp $
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "bufstring.h"
#include "callback.h"
#include "cubesampling.h"
#include "draw.h"
#include "emposid.h"
#include "multiid.h"
#include "position.h"
#include "refcount.h"

class Color;
class CubeSampling;
class Executor;
class IOObj;
class IOObjContext;
class TaskRunner;

namespace Geometry { class Element; }

template <class T> class Selector;

namespace EM
{
class EMManager;

mClass(EarthModel) EMObjectCallbackData
{
public:
    		EMObjectCallbackData() 
		    : pid0( 0, 0, 0 )
		    , pid1( 0, 0, 0 )
		    , attrib( -1 )
		    , flagfor2dviewer(false)
		    , event( EMObjectCallbackData::Undef )
		{}

    enum Event { Undef, PositionChange, PosIDChange, PrefColorChange, Removal,
   		 AttribChange, SectionChange, BurstAlert } event;

    EM::PosID	pid0;
    EM::PosID	pid1;	//Only used in PosIDChange
    int		attrib; //Used only with AttribChange
    bool	flagfor2dviewer; //Used only with BurstAlert for 2DViewer
};


/*! Iterator that iterates a number of positions (normally all) on an EMObject.
The object is created by EMObject::createIterator, and the next() function is
called until no more positions can be found. */


mClass(EarthModel) EMObjectIterator
{
public:
    virtual		~EMObjectIterator() {}
    virtual EM::PosID	next() 		= 0;
    			/*!<posid.objectID()==-1 when there are no more pids*/
    virtual int		approximateSize() const	{ return maximumSize(); }
    virtual int		maximumSize() const	{ return -1; }
    virtual bool	canGoTo() const		{ return false; }
    virtual EM::PosID	goTo(od_int64)		{ return EM::PosID(-1,-1,-1); }
};


mClass(EarthModel) PosAttrib
{
public:
    			PosAttrib()
			    : style_(MarkerStyle3D::Cube,4,Color::White()) {}

    enum Type		{ PermanentControlNode, TemporaryControlNode,
			  EdgeControlNode, TerminationNode, SeedNode };

    Type		type_;
    TypeSet<PosID>	posids_;

    MarkerStyle3D	style_;

    bool		locked_;    
};



/*!\brief Earth Model Object */

mClass(EarthModel) EMObject : public CallBacker
{
mRefCountImplWithDestructor(EMObject,virtual ~EMObject(),
{ prepareForDelete(); delete this; } );
public:
    const ObjectID&		id() const		{ return id_; }
    virtual const char*		getTypeStr() const			= 0;
    const MultiID&		multiID() const		{ return storageid_; }
    void			setMultiID(const MultiID&);

    virtual bool		isOK() const		{ return true; }

    void			setName( const char* nm )  { objname_ = nm; }
    				/*!<The IOObj name overrules this */
    BufferString		name() const;
    virtual void		setNewName();

    virtual int			nrSections() const 			= 0;
    virtual SectionID		sectionID(int) const			= 0;
    virtual BufferString	sectionName(const SectionID&) const;
    virtual bool		canSetSectionName() const;
    virtual bool		setSectionName(const SectionID&,const char*,
	    				       bool addtohistory);
    virtual int			sectionIndex(const SectionID&) const;
    virtual bool		removeSection(SectionID,bool hist )
    					{ return false; }

    const Geometry::Element*	sectionGeometry(const SectionID&) const;
    Geometry::Element*		sectionGeometry(const SectionID&);

    const Color&		preferredColor() const;
    void			setPreferredColor(const Color&,
						  bool addtohistory=false);
    void			setBurstAlert(bool yn);
    bool			hasBurstAlert() const;

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
    virtual void		removeSelected(const Selector<Coord3>&,
	    				       TaskRunner*);
    void			removeListOfSubIDs(const TypeSet<EM::SubID>&,
	    					   const EM::SectionID&);
    void			removeAllUnSeedPos();
    const CubeSampling		getRemovedPolySelectedPosBox();
    void			emptyRemovedPolySelectedPosBox();

    CNotifier<EMObject,const EMObjectCallbackData&>	change;

    virtual Executor*		loader()		{ return 0; }
    virtual bool		isLoaded() const	{ return false; }
    virtual Executor*		saver()			{ return 0; }
    virtual bool		isChanged() const	{ return changed_; }
    virtual bool		isEmpty() const;
    virtual void		setChangedFlag()	{ changed_=true; }
    virtual void		resetChangedFlag()	{ changed_=false; }
    bool			isFullyLoaded() const	{ return fullyloaded_; }
    void			setFullyLoaded(bool yn) { fullyloaded_=yn; }

    virtual bool		isLocked() const	{ return locked_; }
    virtual void		lock(bool yn)		{ locked_=yn;}

    bool			isInsideSelRemoval() const
				{ return insideselremoval_; }
    bool			isSelRemoving() const	{ return selremoving_; }

    const char*			errMsg() const;
    void			setErrMsg(const char* m) { errmsg_ = m; }

    virtual bool		usePar( const IOPar& );
    virtual void		fillPar( IOPar& ) const;

    static int			sPermanentControlNode();
    static int			sTemporaryControlNode();
    static int			sEdgeControlNode();
    static int			sTerminationNode();
    static int			sSeedNode();

    virtual const IOObjContext&	getIOObjContext() const = 0;

protected:
    				EMObject( EMManager& );
    				//!<must be called after creation
    virtual Geometry::Element*	sectionGeometryInternal(const SectionID&);
    virtual void		prepareForDelete() const;
    void			posIDChangeCB(CallBacker*);

    BufferString		objname_;
    ObjectID			id_;
    MultiID			storageid_;
    class EMManager&		manager_;
    BufferString		errmsg_;

    Color&			preferredcolor_;

    ObjectSet<PosAttrib>	posattribs_;
    TypeSet<int>		attribs_;

    CubeSampling		removebypolyposbox_;

    bool			changed_;
    bool			fullyloaded_;
    bool			locked_;
    int				burstalertcount_;

    bool			insideselremoval_;
    bool			selremoving_; 

    static const char*		prefcolorstr();
    static const char*		nrposattrstr();
    static const char*		posattrprefixstr();
    static const char*		posattrsectionstr();
    static const char*		posattrposidstr();
    static const char*		markerstylestr();
};


}; // Namespace

#define mDefineEMObjFuncs( clss ) \
public: \
				clss(EM::EMManager&); \
    static void			initClass(); \
    static EMObject*		create(EM::EMManager&); \
    static clss*		create(const char* nm); \
    static const char*		typeStr(); \
    const char*			getTypeStr() const; \
    void			setNewName(); \
protected: \
				~clss()

#define mImplementEMObjFuncs( clss, typenm ) \
void clss::initClass() \
{ \
    EMOF().addCreator( create, typeStr() ); \
} \
 \
 \
EMObject* clss::create( EM::EMManager& emm ) \
{ \
    EMObject* obj = new clss( emm ); \
    if ( !obj ) return 0; \
    obj->ref();         \
    emm.addObject( obj ); \
    obj->unRefNoDelete(); \
    return obj; \
} \
\
clss* clss::create( const char* nm ) \
{ \
    const ObjectID objid = EMM().createObject( typeStr(), nm ); \
    EMObject* emobj = EMM().getObject( objid ); \
    mDynamicCastGet(clss*,newobj,emobj); \
    return newobj; \
} \
\
const char* clss::typeStr() { return typenm; } \
const char* clss::getTypeStr() const { return typeStr(); } \
void clss::setNewName() \
{\
    static int objnr = 1; \
    BufferString nm( "<New ", typenm, " " ); \
    nm.add( objnr++ ).add( ">" ); \
    setName( nm ); \
}

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
  history matters are handled by the Undo objects.

*/

#endif

