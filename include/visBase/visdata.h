#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "visdataman.h"
#include "sets.h"
#include "namedobj.h"

namespace visBase { class EventInfo; }

namespace osg { class Switch; class Node; class StateSet; }
namespace osgViewer { class CompositeViewer; }
namespace osgGeo { class GLInfo; }

#define mVisTrans visBase::Transformation


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
inline unsigned int cEventTraversalMask()		{ return 0x00000001; }
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

mExpClass(visBase) DataObject	: public RefCount::Referenced
				, public NamedCallBacker
{
public:

    virtual const char*		getClassName() const	= 0;

    virtual bool		isOK() const		{ return true; }

    int				id() const		{ return id_; }

    void			setID(int nid);
    static int			getID(const osg::Node*);

    virtual BufferString	getName() const;
    virtual const OD::String&	name() const;
    virtual void		setName(const char*);
    uiString			uiName() const;
    void			setUiName(const uiString&);

    osg::Node*			osgNode(bool skipswitch=false);
    const osg::Node*		osgNode(bool skipswitch=false) const;

    void			enableTraversal(unsigned int mask,bool yn=true);
    bool			isTraversalEnabled(unsigned int mask) const;

    virtual bool		turnOn(bool yn);
    virtual bool		isOn() const;

    bool			isPickable(bool actively=true) const;
    void			setPickable(bool actively,bool passively=true);
				/*!<actively: mouse click/drag, key press, etc.
				    passively: hovering (e.g. status info) */

    virtual bool		rightClickable() const	{ return selectable(); }
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
    virtual NotifierAccess*	selection()		{ return 0; }
    virtual NotifierAccess*	deSelection()		{ return 0; }
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
				/*!<Sets whether the coord system is
				    right or left handed. */
    virtual bool		isRightHandSystem() const	{ return true; }

    virtual void		setPixelDensity(float dpi);
    static float		getDefaultPixelDensity();
    virtual float		getPixelDensity() const;

    virtual const uiString&	errMsg() const
					    { return uiString::empty(); }

    bool			serialize(const char* filename,
					  bool binary=false);

    void			setParent( DataObjectGroup* p ) { parent_ = p; }
    DataObjectGroup*		parent()		{ return parent_; }
    const DataObjectGroup*	parent() const		{ return parent_; }
    virtual Scene*		scene()			{ return gtScene(); }
    virtual const Scene*	scene() const		{ return gtScene(); }

    template <class T> T*	addNodeState(T* ns)
				{ doAddNodeState(ns); return ns; }
    NodeState*			removeNodeState(NodeState*);
    NodeState*			getNodeState( int idx );

    static void			setVisualizationThread(Threads::ThreadID);
				//!<Call only once from initialization
    static bool			isVisualizationThread();

    static void			requestSingleRedraw();

    static void			setCommonViewer(osgViewer::CompositeViewer*);
    static osgViewer::CompositeViewer* getCommonViewer();


    static const osgGeo::GLInfo* getGLInfo();
				/*!<Captures information of the graphics card.
				    Must be run during a render. at least once
				 */
    static NotifierAccess&	glInfoAvailable() { return glinfoavailable_; }
				//Triggers once, when glinfo is available

protected:

				~DataObject();

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
				    setOsgNodeInternal( (osg::Node*)t );
				    return t;
				}
				//!<Must be called during construction.

    void			updateNodemask();
    virtual Scene*		gtScene() const;

private:

    void				setOsgNodeInternal(osg::Node*);
    void				updateOsgNodeData();

    ObjectSet<NodeState>		nodestates_;
    osg::Node*				osgnode_;
    osg::Switch*			osgoffswitch_;
    int					id_;
    bool				ison_;
    unsigned int			enabledmask_;
    uiString				uiname_;
    static Threads::ThreadID		visualizationthread_;
    static osgViewer::CompositeViewer*	commonviewer_;
    static Notifier<DataObject>		glinfoavailable_;

};

};

#define mCreateDataObj(clss)					\
{								\
    return new clss;						\
}								\
								\
private:							\
    static visBase::DataObject* createInternal()		\
				{ return new clss; }		\
    clss&			operator =(const clss&);	\
				clss(const clss&);		\
public:								\
	clss();	\
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
