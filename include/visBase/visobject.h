#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visobject.h,v 1.5 2002-02-26 17:54:40 kristofer Exp $
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
			VisualObject(Scene&);
    virtual		~VisualObject();
    void		turnOn(bool);
    bool		isOn() const;

    void		setColor(const Color& );

    virtual void	switchColorMode( bool totable );
    bool		isColorTable() const;

    virtual void	setColorTable( ColorTable* );
    			/*!< I will take over you */
    const ColorTable&	colorTable() const;
    ColorTable&		colorTable();

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
