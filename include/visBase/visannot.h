#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visosg.h"
#include "visobject.h"
#include "color.h"
#include "trckeyzsampling.h"
#include "position.h"
#include "sets.h"

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
class Text2;
class DataObjectGroup;

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

mExpClass(visBase) Annotation : public VisualObjectImpl
{
public:
    static Annotation*	create()
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
    void		setScale(const TrcKeyZSampling&);
			/*<! TrcKeyZSampling(false) as input takes default
			     axis scale of survey box instead */
    const TrcKeyZSampling& getScale() const;

    void		setScaleFactor(int dim,int scale);

    void		setText(int dim,const uiString&);
    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    void		setDisplayTransformation(const mVisTrans*) override;
    const mVisTrans*	getDisplayTransformation() const override
			{ return displaytrans_; }
    virtual void	setPixelDensity(float) override;
    const Text2*	getAnnotTexts() const { return axisannot_; }


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

    int				scalefactor_[3];

    TrcKeyZSampling		tkzs_;
    TrcKeyZSampling		tkzsdefaultscale_;
    TrcKeyZSampling		scale_;
    osg::Geometry*		box_;
    osg::Array*			gridlinecoords_;
    osg::Geode*			geode_;
    osgGeo::OneSideRender*	gridlines_;
    RefMan<Text2>		axisnames_;
    RefMan<Text2>		axisannot_;
    visBase::Scene*		scene_;

    const mVisTrans*		displaytrans_;

    OD::Color			annotcolor_;
    bool			allowshading_;

    static const char*		textprefixstr();
    static const char*		cornerprefixstr();
    static const char*		showtextstr();
    static const char*		showscalestr();
};

} // namespace visBase

