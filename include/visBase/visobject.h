#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visobject.h,v 1.1 2002-02-06 22:30:19 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

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

    void		setColor(float, float, float);

    virtual void	switchColorMode( bool totable );
    bool		isColorTable() const;
    ColorTable*		colorTable() { return colortable; }
    SoNode*		getData();

protected:

    void		updateMaterial();
    float		color[3];
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
    bool			usecolortable;

};

};


#endif
