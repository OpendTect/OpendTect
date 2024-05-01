#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "uistring.h"
#include "visdataman.h"
#include "visnodestate.h"

class SoNode;
class BufferString;

namespace osg { class Node; class StateSet; class Switch; }
namespace osgViewer { class CompositeViewer; }


#define mVisTrans visBase::Transformation

namespace visBase
{

class DataManager;
class EventInfo;
class Scene;
class SelectionManager;
class Transformation;


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

mExpClass(visBase) DataObject : public SharedObject
{
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    virtual bool		isOK() const		{ return true; }

    VisID			id() const		{ return id_; }

    void			setID(const VisID&);
    static VisID		getID(const osg::Node*);

    BufferString		getName() const override;
    const OD::String&		name() const override;
    void			setName(const char*) override;
    uiString			uiName() const;
    void			setUiName(const uiString&);
    mDeprecated("Use setUiName") void setName( const uiString& uistr )
				{ setUiName(uistr); }

    osg::Node*			osgNode(bool skipswitch=false);
    const osg::Node*		osgNode(bool skipswitch=false) const;

    void			enableTraversal(unsigned int mask,bool yn=true);
    bool			isTraversalEnabled(unsigned int mask) const;

    inline SoNode*		getInventorNode()	{ return nullptr; }
    inline const SoNode*	getInventorNode() const	{ return nullptr; }

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
    virtual NotifierAccess*	selection()		{ return nullptr; }
    virtual NotifierAccess*	deSelection()		{ return nullptr; }
    virtual NotifierAccess*	rightClicked()		{ return nullptr; }
    virtual const TypeSet<VisID>* rightClickedPath() const{ return nullptr; }

    virtual void		setDisplayTransformation(const mVisTrans*);
				/*!< All positions going from the outside
				     world to the vis should be transformed
				     with this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual const mVisTrans*	getDisplayTransformation() const
				{ return nullptr; }
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

    static float		getDefaultPixelDensity();
    static void			setDefaultPixelDensity(float);
				/* Will only affect objects to be created */

    virtual void		setPixelDensity(float dpi);
    virtual float		getPixelDensity() const;

    virtual const char*		errMsg() const	{ return nullptr; }

    bool			serialize(const char* filename,
					  bool binary=false);

    template <class T> T*	addNodeState( T* ns )
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
				DataObject();
				~DataObject();

    virtual osg::StateSet*	getStateSet();
    void			doAddNodeState(NodeState*);

    friend class		SelectionManager;
    friend class		Scene;
    virtual void		triggerSel()				{}
				/*!<Is called everytime object is selected.*/
    virtual void		triggerDeSel()				{}
				/*!<Is called everytime object is deselected.*/
    virtual void		triggerRightClick(const EventInfo* =nullptr)
				{}

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

    RefObjectSet<NodeState>		nodestates_;
    osg::Node*				osgnode_ = nullptr;
    osg::Switch*			osgoffswitch_ = nullptr;
    VisID				id_;
    bool				ison_	= true;
    uiString				uiname_;
    unsigned int			enabledmask_;
    static const void*			visualizationthread_;
    static osgViewer::CompositeViewer*	commonviewer_;
};

} // namespace visBase

#define mCreateDataObjImpl(clss)				\
RefMan<clss> clss::create()					\
{								\
    RefMan<clss> ret = new clss;				\
    return ret;							\
}								\

#define mCreateDataObj(clss)					\
private:							\
				clss();				\
				mOD_DisableCopy(clss);		\
    static visBase::DataObject* createInternal()		\
				{ return new clss; }		\
public:								\
    static void			initClass();			\
    static const char*		getStaticClassName();		\
    static const char*		sFactoryKeyword();		\
    const char*			getClassName() const override


#define mCreateFactoryEntry( clss )				\
mCreateDataObjImpl( clss )					\
const char* clss::getStaticClassName() { return #clss; }	\
const char* clss::getClassName() const				\
{ return clss::getStaticClassName(); }				\
const char* clss::sFactoryKeyword() { return #clss; }		\
void clss::initClass()						\
{ visBase::DataManager::factory().addCreator(			\
		    createInternal, getStaticClassName() ); }
