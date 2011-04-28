#ifndef vismaterial_h
#define vismaterial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismaterial.h,v 1.17 2011-04-28 07:00:12 cvsbert Exp $
________________________________________________________________________


-*/

#include "visdata.h"
#include "color.h"

class SoMaterial;

namespace visBase
{

/*!\brief


*/

mClass Material : public DataObject
{
public:
    static Material*	create()
			mCreateDataObj(Material);

    Notifier<Material>	change;

    void		setFrom(const Material&);

    void		setColor(const Color&,int=0);
    const Color&	getColor(int matnr=0) const;

    void		setDiffIntensity(float,int=0);
			/*!< Should be between 0 and 1 */
    float		getDiffIntensity(int=0) const;

    void		setAmbience(float);
			/*!< Should be between 0 and 1 */
    float		getAmbience() const;

    void		setSpecIntensity(float);
			/*!< Should be between 0 and 1 */
    float		getSpecIntensity() const;

    void		setEmmIntensity(float);
			/*!< Should be between 0 and 1 */
    float		getEmmIntensity() const;

    void		setShininess(float);
			/*!< Should be between 0 and 1 */
    float		getShininess() const;

    void		setTransparency(float);
			/*!< Should be between 0 and 1 */
    float		getTransparency() const;

    void		setDisplayTransformation(Transformation*) {}
    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:
			~Material();
    void		setMinNrOfMaterials(int);
    void		updateMaterial(int);

    TypeSet<Color>	color_;
    TypeSet<float>	diffuseintencity_;
    float		ambience_;
    float		specularintensity_;
    float		emmissiveintensity_;
    float		shininess_;
    float		transparency_;

    SoMaterial*		material_;

    static const char*	sKeyColor();
    static const char*	sKeyAmbience();
    static const char*	sKeyDiffIntensity();
    static const char*	sKeySpectralIntensity();
    static const char*	sKeyEmmissiveIntensity();
    static const char*	sKeyShininess();
    static const char*	sKeyTransparency();

    virtual SoNode*	gtInvntrNode();

};

} // namespace visBase


#endif
