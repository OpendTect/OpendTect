#ifndef visdata_h
#define visdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdata.h,v 1.65 2012-08-03 13:01:23 cvskris Exp $
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "callback.h"
#include "refcount.h"
#include "sets.h"
#include "visdataman.h"

class SoNode;
class IOPar;
class BufferString;

namespace visBase { class DataObject; class EventInfo; }

namespace osg { class Node; }


#define mVisTrans visBase::Transformation


namespace visBase
{

class Transformation;
class SelectionManager;
class DataManager;
class Scene;


// OSG traversal bitmasks defined by OpendTect
enum TraversalType
{
    EventTraversal		=	0x00000001,
    IntersectionTraversal	=	0x00000002
}; 


/*!\brief
DataObject is the base class off all objects that are used in Visualisation and
ought to be shared in visBase::DataManager. The DataManager owns all the
objects and is thus the only one that is allowed to delete it. The destructors
on the inherited classes should thus be protected.
*/

mClass(visBase) DataObject : public CallBacker
{ mRefCountImpl(DataObject);
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    static void			setOsg();
    static bool			doOsg();
    
    virtual bool		isOK() const		{ return true; }

    int				id() const		{ return id_; }
    void			setID(int nid);

    const char*			name() const;
    virtual void		setName(const char*);

    osg::Node*			osgNode()		{return gtOsgNode();}
    const osg::Node*		osgNode() const
				    { return const_cast<DataObject*>(this)->
							gtOsgNode(); }

    void			enableTraversal(TraversalType,bool yn=true); 
    bool			isTraversalEnabled(TraversalType) const;

    inline SoNode*		getInventorNode()	{return gtInvntrNode();}
    inline const SoNode*	getInventorNode() const
				{ return const_cast<DataObject*>(this)->
							gtInvntrNode(); }

    virtual bool		pickable() const 	{ return selectable(); }
    virtual bool		rightClickable() const 	{ return selectable(); }
    virtual bool		selectable() const	{ return false; }
    void			select() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    void			deSelect() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    virtual bool		isSelected() const;
    virtual NotifierAccess*	selection() 		{ return 0; }
    virtual NotifierAccess*	deSelection() 		{ return 0; }
    virtual NotifierAccess*	rightClicked()		{ return 0; }
    virtual const TypeSet<int>*	rightClickedPath() const{ return 0; }

    virtual void		setDisplayTransformation(const mVisTrans*);
    				/*!< All positions going from the outside
				     world to the vis should be transformed
				     witht this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual const mVisTrans*	getDisplayTransformation() const { return 0; }
    				/*!< All positions going from the outside
				     world to the vis should be transformed
				     witht this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual void		setRightHandSystem(bool yn)	{}
    				/*!<Sets whether the coordinate system is
				    right or left handed. */
    virtual bool		isRightHandSystem() const	{ return true; }

    virtual int			usePar(const IOPar&);
    				/*!< Returns -1 on error and 1 on success.
				     If it returns 0 it is missing something.
				     Parse everything else and retry later.  */
    virtual const char*		errMsg() const	{ return 0; }

    virtual bool		acceptsIncompletePar() const    {return false;}
				/*!<Returns true if it can cope with non-
				    complete session-restores. If it returns
				    false, DataManager::usePar() will
				    fail if usePar of this function returns
				    0, and it doesn't help to retry.  */

    virtual void		fillPar(IOPar&, TypeSet<int>&) const;

    void			doSaveInSessions( bool yn )
				{ saveinsessions_ = yn; }
    bool			saveInSessions() const
    				{ return saveinsessions_; }
			
    bool			serialize(const char* filename,
	    				  bool binary=false);

protected:
    friend class		SelectionManager;
    friend class		Scene;
    virtual void		triggerSel()				{}
    				/*!<Is called everytime object is selected.*/
    virtual void		triggerDeSel()				{}
    				/*!<Is called everytime object is deselected.*/
    virtual void		triggerRightClick(const EventInfo* =0)	{}
    
				DataObject();
    virtual bool		_init();

    bool			saveinsessions_;

    virtual SoNode*		gtInvntrNode()		{ return 0; }

    virtual osg::Node*		gtOsgNode()		{ return 0; }

    void			updateOsgNodeData();

private:
    int				id_;
    BufferString*		name_;

    static bool			doosg_;
};

};

#define _mCreateDataObj(clss) 					\
{								\
    clss* res = (clss*) createInternal();			\
    return res;							\
}								\
								\
private:							\
    static visBase::DataObject* createInternal();		\
    clss(const clss&);						\
    clss& operator =(const clss&);				\
public:								\
    static void		initClass();				\
    static const char*	getStaticClassName();			\
								\
    virtual const char*	getClassName() const; 			\
protected:
    
#define _mDeclConstr(clss)					\
    clss();							\
public:

#define mCreateDataObj(clss) 					\
    _mCreateDataObj(clss) 					\
    _mDeclConstr(clss)


#define mImplVisInitClass( clss ) \
void clss::initClass()						\
{								\
    visBase::DataManager::factory().addCreator(			\
	clss::createInternal, #clss );				\
}

#define mCreateFactoryEntryNoInitClass( clss )			\
const char* clss::getStaticClassName() { return #clss; }	\
const char* clss::getClassName() const				\
{ return clss::getStaticClassName(); }				\
visBase::DataObject* clss::createInternal()			\
{								\
    clss* res = new clss;					\
    if ( !res )							\
	return 0;						\
    if ( !res->_init() || !res->isOK() )			\
    {								\
	delete res;						\
	return 0;						\
    }								\
								\
    return res;							\
}							


#define mCreateFactoryEntry( clss )				\
mImplVisInitClass( clss );					\
mCreateFactoryEntryNoInitClass( clss );		




#endif

