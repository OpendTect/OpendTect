#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "visnodestate.h"
#include "color.h"
#include "notify.h"
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
    void		setColors(const TypeSet<Color>&,
				  bool trigger = true);
			/*!< set material's od colors by input colors. */


    enum ColorMode	{ Ambient, Diffuse, Specular, Emission,
			  AmbientAndDiffuse, Off };

    void		setColorMode( ColorMode );
    ColorMode		getColorMode() const;

    void		setColor(const Color&,int=-1,bool trigger=true);
			/*!< set material's od colors by input colors.
			using setColors() to instead of this calling
			if having to setColor many times. */
    Color		getColor(int matnr=0) const;

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

    const TypeSet<Color> getColors();

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

    Color		color_;
    float		ambience_;
    float		specularintensity_;
    float		emmissiveintensity_;
    float		shininess_;
    float		diffuseintensity_;

    unsigned int	colorbindtype_;

    mutable Threads::Lock	lock_;
    /*!< the lock will protect below variables */
    osg::Array*			osgcolorarray_;
    ObjectSet<osg::Geometry>	attachedgeoms_;
    float			transparencybendpower_;
};

} // namespace visBase
