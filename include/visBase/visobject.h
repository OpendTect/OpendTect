#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visobject.h,v 1.2 2002-02-07 14:15:33 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"
#include "color.h"

class SoSeparator;
class SoNode;
class SoSwitch;
class SoDrawStyle;
class SoMaterial;

namespace visBase
{

class ColorTable;

/*!\brief
    Base class for all objects that are visual on the scene.
*/

class VisualObject : public SceneObject
{
public:
			VisualObject();
    virtual		~VisualObject();
    void		on();
    void		off();
    bool		isOn() const;

    void		setColor(const Color& );

    virtual void	switchColorMode( bool totable );
    bool		isColorTable() const;
    ColorTable*		colorTable() { return colortable; }
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

};

};


#endif
