#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visobject.h,v 1.7 2002-02-28 08:04:59 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"
#include "color.h"

class SoSeparator;
class SoNode;
class SoSwitch;
class SoDrawStyle;
class SoMaterial;

class ColorTable;

namespace visBase
{
class Scene;

/*!\brief
    Base class for all objects that are visual on the scene.
*/

class VisualObject : public SceneObject
{
public:
    virtual		~VisualObject() {}
    virtual void	turnOn(bool)					= 0;
    virtual bool	isOn() const					= 0;

    virtual bool	regForSelection(const VisualObject* assoc=0)	= 0;
    			/*! If it should be possible to select this
			    object, you must register it. Some objects
			    are not possible to select. They will return
			    false;
			*/

    virtual void	setColor(const Color& )				= 0;

    virtual void	switchColorMode( bool totable )			= 0;
    virtual bool	isColorTable() const				= 0;

    virtual const ColorTable&	colorTable() const			= 0;
    virtual ColorTable&		colorTable()				= 0;
    				/*!< if you change the ct,
				     you must notify the object
				     with colorTableChanged()
				*/

    virtual void		colorTableChanged()			= 0;
};


class VisualObjectImpl : public VisualObject
{
public:
			VisualObjectImpl(Scene&);
    virtual		~VisualObjectImpl();
    void		turnOn(bool);
    bool		isOn() const;

    virtual bool	regForSelection(const VisualObject* assoc=0)
			{ return false; }

    void		setColor(const Color& );

    virtual void	switchColorMode( bool totable );
    bool		isColorTable() const;

    const ColorTable&	colorTable() const;
    ColorTable&		colorTable();
    virtual void	colorTableChanged();

    SoNode*		getData();

protected:

    void		updateMaterial();
    Color		color;
    float           	ambience;
    float           	diffuseintencity;
    float		specularintensity;
    float		emmissiveintensity;
    float		shininess;
    float		transparency;

    
    SoSeparator*	root;
    SoSwitch*		onoff;
    SoSwitch*		material;
    SoDrawStyle*	drawstyle;

    SoMaterial*		defaultmaterial;

    ColorTable*		colortable;
    SoMaterial*		colortablematerial;
    bool		usecolortable;

    Scene&		scene;
};

};


#endif
