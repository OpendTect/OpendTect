#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "color.h"
#include "visnodestate.h"
#include "uistring.h"

namespace osg {
    class Material;
    class Array;
    class Geometry;
    class StateSet;
    class StateAttribute;
};

class IOPar;

namespace visBase
{
/*!\brief


*/

mExpClass(visBase) Material : public NodeState
{ mODTextTranslationClass(Material);
public:
			Material();

    Notifier<Material>	change;

    void		setFrom(const Material&, bool trigger= false);

    void		setPropertiesFrom(const Material&, bool trigger= false);
			/*!< set materials by input material's properties */
    void		setColors(const TypeSet<OD::Color>&,
				  bool trigger = true);
			/*!< set material's od colors by input colors. */


    enum ColorMode	{ Ambient, Diffuse, Specular, Emission,
			  AmbientAndDiffuse, Off };

    void		setColorMode( ColorMode );
    ColorMode		getColorMode() const;

    void		setColor(const OD::Color&,int=-1,bool trigger=true);
			/*!< set material's od colors by input colors.
			using setColors() to instead of this calling
			if having to setColor many times. */
    OD::Color		getColor(int matnr=0) const;

    void		removeColor(int idx);

    void		setDiffIntensity(float);
			/*!< Should be between 0 and 1 */
    float		getDiffIntensity() const;

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

    void		setTransparency(float,int idx=0,bool updt=false);
			/*!< Should be between 0 and 1 */
    void		setAllTransparencies( float n );
			/*!< Should be between 0 and 1 */

    void		setTransparencies(float,const Interval<int>& range);

    float		getTransparency(int idx=0) const;

    void		rescaleTransparency(float bendpower);

    int			usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    int			nrOfMaterial() const;

    void		clear();

    void		attachGeometry(osg::Geometry*);
    void		detachGeometry(osg::Geometry*);

    void		setColorBindType(unsigned int);
    
    const TypeSet<OD::Color> getColors();

private:
			~Material();
			//!Used when no array is present
   void			updateOsgMaterial();

    void		createOsgColorArray(int);
    void		setColorArray(osg::Array*);
    void		removeOsgColor(int);
			//!Assumes object is write-locked
    float		getRescaledTransparency() const;

    static const char*	sKeyColor();
    static const char*	sKeyAmbience();
    static const char*	sKeyDiffIntensity();
    static const char*	sKeySpectralIntensity();
    static const char*	sKeyEmmissiveIntensity();
    static const char*	sKeyShininess();
    static const char*	sKeyTransparency();

    friend class	OsgColorArrayUpdator;

    osg::Material*	material_;

    OD::Color		color_;
    float		ambience_;
    float		specularintensity_;
    float		emmissiveintensity_;
    float		shininess_;
    float		diffuseintensity_;

    unsigned int	colorbindtype_;

    mutable Threads::Lock lock_;
    /*!< the lock will protect below variables */
    osg::Array*			osgcolorarray_;
    ObjectSet<osg::Geometry>	attachedgeoms_;
    float			transparencybendpower_;
};

} // namespace visBase
