#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "callback.h"
#include "refcount.h"
#include "sets.h"
#include "visdataman.h"

class SoNode;
class BufferString;

namespace visBase { class DataObject; class EventInfo; }

namespace osg { class Switch; class Node; }
namespace osgViewer { class CompositeViewer; }


#define mVisTrans visBase::Transformation

namespace osg { class Switch; class StateSet; }
namespace visBase
{

class Transformation;
class SelectionManager;
class DataManager;
class Scene;
class DataObjectGroup;
class NodeState;


// OSG traversal bitmasks defined by OpendTect
inline unsigned int cNoTraversalMask()			{ return 0; }
inline unsigned int cAllTraversalMask()			{ return 0xFFFFFFFF; }
inline unsigned int cEventTraversalMask() 		{ return 0x00000001; }
inline unsigned int cBBoxTraversalMask()		{ return 0x00000002; }

inline unsigned int cActiveIntersecTraversalMask()	{ return 0x00000004; }
inline unsigned int cPassiveIntersecTraversalMask()	{ return 0x00000008; }
inline unsigned int cIntersectionTraversalMask()
{ return cActiveIntersecTraversalMask() | cPassiveIntersecTraversalMask(); }
inline unsigned int cDraggerIntersecTraversalMask()	{ return 0x00000010; }


/*!\brief
DataObject is the base class off all objects that are used in Visualization and
ought to be shared in visBase::DataManager. The DataManager owns all the
objects and is thus the only one that is allowed to delete it. The destructors
on the inherited classes should thus be protected.
*/

mExpClass(visBase) DataObject : public CallBacker
{ mRefCountImpl(DataObject);
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    virtual bool		isOK() const		{ return true; }

    int				id() const		{ return id_; }

    void			setID(int nid);
    static int			getID(const osg::Node*);

    uiString			name() const;
    virtual void		setName(const uiString&);

    osg::Node*			osgNode(bool skipswitch=false);
    const osg::Node*		osgNode(bool skipswitch=false) const;

    void			enableTraversal(unsigned int mask,bool yn=true);
    bool			isTraversalEnabled(unsigned int mask) const;

    inline SoNode*		getInventorNode()	{return 0;}
    inline const SoNode*	getInventorNode() const
				{ return 0; }

    virtual bool		turnOn(bool yn);
    virtual bool		isOn() const;

    bool			isPickable(bool actively=true) const;
    void			setPickable(bool actively,bool passively=true);
				/*!<actively: mouse click/drag, key press, etc.
				    passively: hovering (e.g. status info) */

    virtual bool		rightClickable() const 	{ return selectable(); }
    virtual bool		selectable() const	{ return false; }
    void			select() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    void			deSelect() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    void			updateSel() const;
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
				     with this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual const mVisTrans*	getDisplayTransformation() const { return 0; }
    				/*!< All positions going from the outside
				     world to the vis should be transformed
				     with this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual void		setRightHandSystem(bool yn)	{}
    				/*!<Sets whether the coordinate system is
				    right or left handed. */
    virtual bool		isRightHandSystem() const	{ return true; }

    virtual void		setPixelDensity(float dpi);
    static float		getDefaultPixelDensity();
    virtual float		getPixelDensity() const;

    virtual const char*		errMsg() const	{ return 0; }

    bool			serialize(const char* filename,
	    				  bool binary=false);

    void			setParent(DataObjectGroup* g) { parent_ = g; }

    template <class T> T*	addNodeState(T* ns)
				{ doAddNodeState(ns); return ns; }
    NodeState*			removeNodeState(NodeState*);
    NodeState*			getNodeState( int idx );

    static void			setVisualizationThread(const void*);
				//!<Call only once from initialization
    static bool			isVisualizationThread();

    static void			requestSingleRedraw();

    static void			setCommonViewer(osgViewer::CompositeViewer*);
    static osgViewer::CompositeViewer* getCommonViewer();

protected:

    virtual osg::StateSet*	getStateSet();
    void			doAddNodeState(NodeState* ns);

    friend class		SelectionManager;
    friend class		Scene;
    virtual void		triggerSel()				{}
    				/*!<Is called everytime object is selected.*/
    virtual void		triggerDeSel()				{}
    				/*!<Is called everytime object is deselected.*/
    virtual void		triggerRightClick(const EventInfo* =0)	{}
    
				DataObject();

    DataObjectGroup*		parent_;

    template <class T>
    T*				setOsgNode( T* t )
				{
				    setOsgNodeInternal( (osg::Node*) t );
				    return t;
				}
				//!<Must be called during construction.

    void			updateNodemask();

private:
    void			setOsgNodeInternal(osg::Node*);
    void			updateOsgNodeData();

    ObjectSet<NodeState>		nodestates_;
    osg::Node*				osgnode_;
    osg::Switch*			osgoffswitch_;
    int					id_;
    bool				ison_;
    uiString				name_;
    unsigned int			enabledmask_;
    static const void*			visualizationthread_;
    static osgViewer::CompositeViewer*	commonviewer_;
};

};

#define mCreateDataObj(clss) 					\
{								\
    return new clss;						\
}								\
								\
private:							\
    static visBase::DataObject* createInternal()		\
				{ return new clss; }		\
    clss& 			operator =(const clss&);	\
    				clss(const clss&);		\
public:								\
                        	clss();                        	\
    static void			initClass();			\
    static const char*		getStaticClassName();		\
    static const char*		sFactoryKeyword();		\
    virtual const char*		getClassName() const


#define mCreateFactoryEntry( clss )				\
const char* clss::getStaticClassName() { return #clss; }	\
const char* clss::getClassName() const				\
{ return clss::getStaticClassName(); }				\
const char* clss::sFactoryKeyword() { return #clss; }		\
void clss::initClass()						\
{ visBase::DataManager::factory().addCreator(			\
		    createInternal, getStaticClassName() ); }



