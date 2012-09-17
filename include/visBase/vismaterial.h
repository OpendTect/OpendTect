#ifndef vismaterial_h
#define vismaterial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismaterial.h,v 1.20 2011/12/16 15:57:20 cvskris Exp $
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

    void		setTransparency(float,int idx=0);
			/*!< Should be between 0 and 1 */
    float		getTransparency(int idx=0) const;

    void		setDisplayTransformation(const mVisTrans*) {}
    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

    int			nrOfMaterial() const;

protected:
			~Material();
    void		setMinNrOfMaterials(int);
    void		updateMaterial(int);

    TypeSet<Color>	color_;
    TypeSet<float>	diffuseintencity_;
    TypeSet<float>	transparency_;
    
    float		ambience_;
    float		specularintensity_;
    float		emmissiveintensity_;
    float		shininess_;

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
