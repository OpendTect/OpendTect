#ifndef vismaterial_h
#define vismaterial_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismaterial.h,v 1.1 2002-03-11 10:46:12 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class SoMaterial;
class Color;

namespace visBase
{

/*!\brief


*/

class Material : public SceneObject
{
public:
    static Material*	create()
			mCreateDataObj0arg(Material);

    void		setColor( const Color& nc );
    const Color&	getColor() const { return color; }

    void		setAmbience( float );
    float		getAmbience() const { return ambience; }

    void		setDiffIntensity( float );
    float		getDiffIntensity() const { return diffuseintencity; }

    void		setSpecIntensity( float );
    float		getSpecIntensity() const { return specularintensity; }

    void		setEmmIntensity( float );
    float		getEmmIntensity() const { return emmissiveintensity; }

    void		setShininess( float );
    float		getShininess() const { return shininess; }

    void		setTransparency( float );
    float		getTransparency() const { return transparency; }

    SoNode*		getData();

protected:
			Material();
			~Material();

    void		updateMaterial();

    Color&		color;
    float               ambience;
    float               diffuseintencity;
    float               specularintensity;
    float               emmissiveintensity;
    float               shininess;
    float               transparency;

    SoMaterial*		material;
};

}; // Namespace


#endif
