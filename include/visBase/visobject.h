#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visobject.h,v 1.28 2004-05-15 13:15:49 kristofer Exp $
________________________________________________________________________


-*/

#include "visdata.h"
#include "color.h"

class SoSeparator;
class SoNode;
class SoSwitch;

namespace visBase
{
class Material;
class Transformation;
class EventCatcher;

/*!\brief Base class for all objects that are visual on the scene. */

class VisualObject : public DataObject
{
public:
    virtual void		turnOn(bool)				= 0;
    virtual bool		isOn() const				= 0;

    virtual void		setMaterial( Material* )		= 0;
    virtual Material*		getMaterial()				= 0;

    void			setSelectable(bool yn)	{ isselectable=yn; }
    bool			selectable() const 	{ return isselectable; }
    NotifierAccess*		selection() 		{ return &selnotifier; }
    NotifierAccess*		deSelection() 		{return &deselnotifier;}
    virtual NotifierAccess*	rightClicked()		{ return &rightClick; }
    const TypeSet<int>*		rightClickedPath() const{return rightclickpath;}

    virtual int			usePar( const IOPar& iopar )
				{ return DataObject::usePar(iopar); }
    virtual void		fillPar( IOPar& iopar,
	    				 TypeSet<int>& saveids ) const
				{ DataObject::fillPar( iopar, saveids );}

protected:
    void			triggerSel()
    				{ if (isselectable) selnotifier.trigger(); }
    void			triggerDeSel()
    				{ if (isselectable) deselnotifier.trigger(); }
    void			triggerRightClick(const TypeSet<int>* path)
				{
				    rightclickpath = path;
				    rightClick.trigger();
				}

				VisualObject(bool selectable=false);
				~VisualObject();

private:
    bool						isselectable;
    Notifier<VisualObject>				selnotifier;
    Notifier<VisualObject>				deselnotifier;
    Notifier<VisualObject>				rightClick;
    const TypeSet<int>*					rightclickpath;
};


class VisualObjectImpl : public VisualObject
{
public:
    void		turnOn(bool);
    bool		isOn() const;

    void		setMaterial(Material*);
    const Material*	getMaterial() const { return material; }
    Material*		getMaterial() { return material; }

    SoNode*		getInventorNode();

    virtual int		usePar(const IOPar&);
    virtual void	fillPar(IOPar&,TypeSet<int>&) const;

protected:
    void		addChild(SoNode*);
    void		insertChild(int pos,SoNode*);
    void		removeChild(SoNode*);
    bool		isChild(const SoNode*) const;


			VisualObjectImpl(bool selectable=false);
    virtual		~VisualObjectImpl();

    SoSwitch*		onoff;
    Material*		material;

private:
    SoSeparator*	root;

    static const char*	materialidstr;
    static const char*	isonstr;

};

};


/*!\mainpage Visualisation - Open Inventor-based tools

  All 3D visualisation in OpendTect is COIN based. COIN is an implementation
  of the OpenInventor interface sepecification. As usual, the external package
  (i.e. COIN) is not visible to any object outside this module.
  This module can be seen as a layer on top of the COIN library. Compliant with
  the 'DIF' principle (Don't implement the future), the layer only contains
  those tools that are really used in OpendTect.

  The function visBase::initODInventorClasses() initialises our own 
  Inventor classes, which are in fact extensions of the COIN library. 
  These classes can be found is SoOD. The 'vis' classes in this module are 
  used in the rest of OpendTect. 

  In the 3D visualisation world you'll see that it is almost unavoidable that
  all objects will be managed by a centralised manager. That is
  visBase::DataManager. 

  The main classes for displaying scene objects are:
  - visBase::TextureRect, for inlines, crosslines and timeslices.
  - visBase::RandomTrack, for random lines.
  - visBase::CubeView, for the volume viewer.
  - visBase::Marker, for picks.
  - visBase::MeshSurfaceDisplay, for surfaces.
  - visBase::Well, for wells.

*/

#endif
