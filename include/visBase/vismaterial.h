#ifndef vismaterial_h
#define vismaterial_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismaterial.h,v 1.9 2004-07-28 06:55:51 kristofer Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoMaterial;
class Color;

namespace visBase
{

/*!\brief


*/

class Material : public DataObject
{
public:
    static Material*	create()
			mCreateDataObj(Material);

    void		setColor( const Color& nc, int matnr=0 );
    const Color&	getColor(int matnr=0) const;

    void		setAmbience( float, int matnr=0 );
			/*!< Should be between 0 and 1 */
    float		getAmbience(int matnr=0) const;

    void		setDiffIntensity( float, int matnr=0 );
			/*!< Should be between 0 and 1 */
    float		getDiffIntensity(int matnr=0) const;

    void		setSpecIntensity( float, int matnr=0 );
			/*!< Should be between 0 and 1 */
    float		getSpecIntensity(int matnr=0) const;

    void		setEmmIntensity( float, int matnr=0 );
			/*!< Should be between 0 and 1 */
    float		getEmmIntensity(int matnr=0) const;

    void		setShininess( float, int matnr=0 );
			/*!< Should be between 0 and 1 */
    float		getShininess(int matnr=0) const;

    void		setTransparency( float, int matnr=0 );
			/*!< Should be between 0 and 1 */
    float		getTransparency(int matnr=0) const;

    SoNode*		getInventorNode();
    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;

protected:
			~Material();
    void		setMinNrOfMaterials(int minnr);
    void		updateMaterial(int idx);

    TypeSet<Color>	color;
    TypeSet<float>	ambience;
    TypeSet<float>	diffuseintencity;
    TypeSet<float>	specularintensity;
    TypeSet<float>	emmissiveintensity;
    TypeSet<float>	shininess;
    TypeSet<float>	transparency;

    SoMaterial*		material;

    static const char*	colorstr;
    static const char*	ambiencestr;
    static const char*	diffintensstr;
    static const char*	specintensstr;
    static const char*	emmintensstr;
    static const char*	shininessstr;
    static const char*	transpstr;
};

}; // Namespace


#endif
