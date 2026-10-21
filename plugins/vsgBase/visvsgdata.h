#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 dGBVis is the VSG counterpart of visBase. Class and function names follow
 visBase; macros do not, because visBase's macros are visible here too.

-*/

#include "vsgbasemod.h"

#include "color.h"
#include "integerid.h"
#include "notify.h"
#include "sharedobject.h"
#include "thread.h"
#include "uistring.h"

#include <vsg/core/Mask.h>
#include <vsg/core/ref_ptr.h>
#include <vsg/maths/vec4.h>

namespace vsg { class Node; class Object; class Switch; class Viewer; }
namespace vsgGeo { class Updater; }

#define mDGBVisTrans dGBVis::Transformation

namespace dGBVis
{

class DataManager;
class SelectionManager;
class Transformation;

inline constexpr vsg::Mask cActivePickMask = 4;
inline constexpr vsg::Mask cPassivePickMask = 8;
inline constexpr vsg::Mask cRenderMask =
		vsg::MASK_ALL & ~cActivePickMask & ~cPassivePickMask;

inline vsg::vec4 toVec4( const OD::Color& col, float alpha )
{
    return vsg::vec4( col.r()/255.f, col.g()/255.f, col.b()/255.f, alpha );
}


inline vsg::vec4 toVec4( const OD::Color& col )
{
    return toVec4( col, 1.f-col.t()/255.f );
}


/*!\brief Implemented by a viewer that renders on request, so that a
  DataObject can ask for a redraw without knowing about the window system. */

class RedrawRequester
{
public:
    virtual		~RedrawRequester()	{}
    virtual void	request()		= 0;
};


/*!\brief Base class of all dGBVis objects, shared in dGBVis::DM().

  The top node of every object is a vsg::Switch with the object's own node
  as its only child, so turnOn() does not have to change parent groups
  (VSG nodes do not know their parents).
*/

mExpClass(vsgBase) DataObject : public SharedObject
{
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    virtual bool		isOK() const		{ return true; }

    VisID			id() const		{ return id_; }
    void			setID(const VisID&);
    static VisID		getID(const vsg::Node*);

    BufferString		getName() const override;
    const OD::String&		name() const override;
    void			setName(const char*) override;
    uiString			uiName() const;
    void			setUiName(const uiString&);

    vsg::Node*			vsgNode(bool skipswitch=false);
    const vsg::Node*		vsgNode(bool skipswitch=false) const;

    virtual bool		turnOn(bool yn);
				//!<\returns previous state
    virtual bool		isOn() const		{ return ison_; }

    void			setPickable(bool actively,bool passively=true);
				/*!<actively: click and drag. passively: hover.
				    Both are off until a class opts in, so
				    existing objects are not under the
				    cursor. */
    bool			isPickable(bool actively=true) const;

    virtual bool		rightClickable() const	{ return selectable(); }
    virtual bool		selectable() const	{ return false; }
    virtual NotifierAccess*	selection()		{ return nullptr; }
    virtual NotifierAccess*	deSelection()		{ return nullptr; }
    virtual NotifierAccess*	rightClicked()		{ return nullptr; }

    virtual void		setDisplayTransformation(const mDGBVisTrans*);
    virtual const mDGBVisTrans* getDisplayTransformation() const
				{ return nullptr; }

    virtual void		setRightHandSystem(bool)	{}
    virtual bool		isRightHandSystem() const	{ return true; }

    static float		getDefaultPixelDensity();
    static void			setDefaultPixelDensity(float);
    virtual void		setPixelDensity(float)		{}
    virtual float		getPixelDensity() const;

    virtual const char*		errMsg() const	{ return nullptr; }

    static void			setVisualizationThread(Threads::ThreadID);
    static bool			isVisualizationThread();

    static void			setCommonViewer(vsg::Viewer*);
    static Notifier<DataObject>& commonViewerChanged();
				/*!<Triggered when the common viewer is set or
				    reset, after its updater is installed. */
    static vsg::Viewer*		getCommonViewer();
    static vsgGeo::Updater*	getUpdater();
    static void			requestSingleRedraw();

protected:
				DataObject();
				~DataObject();

    friend class		SelectionManager;

    virtual void		triggerSel()			{}
    virtual void		triggerDeSel()			{}
    virtual void		triggerRightClick()		{}

    void			applyNodeMask();

    template <class T>
    T*				setVsgNode( T* t )
				{
				    setVsgNodeInternal( (vsg::Node*)t );
				    return t;
				}

    static void			compileNode(vsg::Object*);
				/*!<For nodes added to a running scene. */
    static void			releaseNode(vsg::Object*);
				/*!<For nodes removed from a running scene:
				    kept alive until the GPU is done. */

private:

    void			setVsgNodeInternal(vsg::Node*);
    void			updateVsgNodeData();

    vsg::ref_ptr<vsg::Switch>	switch_;
    vsg::ref_ptr<vsg::Node>	node_;
    VisID			id_;
    bool			ison_ = true;
    bool			activepick_ = false;
    bool			passivepick_ = false;
    uiString			uiname_;

    static Threads::ThreadID	visualizationthread_;
};

} // namespace dGBVis


#define mCreateDGBVisObjImpl(clss)				\
RefMan<clss> clss::create()					\
{								\
    RefMan<clss> ret = new clss;				\
    return ret;							\
}

#define mCreateDGBVisObj(clss)					\
private:							\
				clss();				\
				mOD_DisableCopy(clss);		\
    static dGBVis::DataObject*	createInternal()		\
				{ return new clss; }		\
public:								\
    static void			initClass();			\
    static const char*		getStaticClassName();		\
    static const char*		sFactoryKeyword();		\
    const char*			getClassName() const override

#define mCreateDGBVisFactoryEntry( clss )			\
mCreateDGBVisObjImpl( clss )					\
const char* clss::getStaticClassName() { return #clss; }	\
const char* clss::getClassName() const				\
{ return clss::getStaticClassName(); }				\
const char* clss::sFactoryKeyword() { return #clss; }		\
void clss::initClass()						\
{ dGBVis::DataManager::factory().addCreator(			\
		    createInternal, getStaticClassName() ); }
