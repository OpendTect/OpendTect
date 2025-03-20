#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "color.h"
#include "position.h"
#include "sets.h"
#include "trckeyzsampling.h"
#include "visosg.h"
#include "visobject.h"
#include "visscene.h"
#include "vistransform.h"

class AxisInfo;
class FontData;
class uiString;

namespace osg
{
    class Geode;
    class Array;
    class Group;
    class Geometry;
    class Vec3f;
}

namespace osgGeo
{
    class OneSideRender;
}

namespace visBase
{
class DataObjectGroup;
class Text2;

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

mExpClass(visBase) Annotation : public VisualObjectImpl
{
public:
    static RefMan<Annotation> create();
			mCreateDataObj(Annotation);

    void		showText(bool yn);
    bool		isTextShown() const;

    void		showScale(bool yn);
    bool		isScaleShown() const;

    bool		canShowGridLines() const;
    void		showGridLines(bool yn);
    bool		isGridLinesShown() const;

    const FontData&	getFont() const;
    void		setFont(const FontData&);

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    const TrcKeyZSampling& getTrcKeyZSampling() const;
    void		setScale(const TrcKeyZSampling&,
				 const double* scalefacs=nullptr, int nrvals=0,
				 bool makescalenice=false);
			mDeprecated("Scale should be an array of double")
    void		setScaleFactor(int dim,int scale);

			/*<! TrcKeyZSampling(false) as input takes default
			     axis scale of survey box instead */
    const TrcKeyZSampling& getScale(bool getdefault=true) const;

    void		setText(int dim,const uiString&);
    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    void		setDisplayTransformation(const mVisTrans*) override;
    const mVisTrans*	getDisplayTransformation() const override
			{ return displaytrans_.ptr(); }
    void		setPixelDensity(float) override;
    const Text2*	getAnnotTexts() const { return axisannot_.ptr(); }

    void		setScene(Scene*);
    void		allowShading(bool yn) { allowshading_=yn; }

private:
			~Annotation();

    void		firstTraversal(CallBacker*);
    void		initGridLines();
    void		updateGridLines();
    void		updateTextPos();
    void		updateTextColor(CallBacker*);
    void		getAxisCoords(int,osg::Vec3f&,osg::Vec3f&) const;
    void		setCorner( int, float, float, float );

    double		scalefactor_[3];

    TrcKeyZSampling		tkzs_;
    TrcKeyZSampling		tkzsdefaultscale_;
    TrcKeyZSampling		scale_;
    osg::Geometry*		box_;
    osg::Array*			gridlinecoords_;
    osg::Geode*			geode_;
    osgGeo::OneSideRender*	gridlines_;
    RefMan<Text2>		axisnames_;
    RefMan<Text2>		axisannot_;
    WeakPtr<Scene>		scene_;

    ConstRefMan<mVisTrans>	displaytrans_;

    OD::Color			annotcolor_;
    bool			allowshading_	= true;

    static const char*		textprefixstr();
    static const char*		cornerprefixstr();
    static const char*		showtextstr();
    static const char*		showscalestr();
};

} // namespace visBase
