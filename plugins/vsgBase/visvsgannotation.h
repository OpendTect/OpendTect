#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgobject.h"

#include "color.h"
#include "fontdata.h"
#include "trckeyzsampling.h"
#include "objectset.h"
#include "typeset.h"
#include "uistring.h"

#include <vsg/core/Array.h>

namespace vsgGeo { class OneSideRender; }

namespace dGBVis
{

class Text2;

/*!\brief Counterpart of visBase::Annotation: a wireframe box with grid lines
  on the sides facing away from the camera, and axis annotation: the axis
  names, and the values at the ticks.*/

mExpClass(vsgBase) Annotation : public VisualObjectImpl
{
public:
    static RefMan<Annotation> create();
			mCreateDGBVisObj(Annotation);

    void		showText(bool yn);
    bool		isTextShown() const		{ return textshown_; }

    void		showScale(bool yn);
    bool		isScaleShown() const		{ return scaleshown_; }

    bool		canShowGridLines() const	{ return true; }
    void		showGridLines(bool yn);
    bool		isGridLinesShown() const	{ return gridshown_; }

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    const TrcKeyZSampling& getTrcKeyZSampling() const	{ return tkzs_; }
    void		setScale(const TrcKeyZSampling&,
				 const double* scalefacs=nullptr,int nrvals=0,
				 bool makescalenice=false);
    const TrcKeyZSampling& getScale(bool getdefault=true) const;

    void		setText(int dim,const uiString&);
    const uiString&	getText(int dim) const;

    void		setColor(const OD::Color&);
    OD::Color		getColor() const		{ return annotcolor_; }

    const FontData&	getFont() const;
    void		setFont(const FontData&);
    void		setPixelDensity(float) override;

    struct Tick
    {
	int		dim_;
	float		value_;
	Coord3		pos_;	//!<Display space

	bool		operator==( const Tick& oth ) const
			{ return dim_==oth.dim_ && value_==oth.value_ &&
				 pos_==oth.pos_; }
    };

    const TypeSet<Tick>& getTicks() const		{ return ticks_; }
    Coord3		getCorner(int idx) const;
    int			nrGridLineSegments(int side) const;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    void		setDisplayTransformation(const mDGBVisTrans*) override;
    const mDGBVisTrans* getDisplayTransformation() const override;

    void		allowShading( bool )			{}

private:
			~Annotation();

    void		setCorner(int,double,double,double);
    void		updateBox();
    void		updateGridLines();
    void		updateTextPos();
    void		updateTextColor();
    void		getAxisCoords(int,vsg::vec3&,vsg::vec3&) const;

    TrcKeyZSampling	tkzs_;
    TrcKeyZSampling	tkzsdefaultscale_;
    TrcKeyZSampling	scale_;
    double		scalefactor_[3];

    vsg::ref_ptr<vsg::vec3Array>	boxcoords_;
    vsg::ref_ptr<vsg::vec4Array>	boxcolor_;
    vsg::ref_ptr<vsg::Node>		boxnode_;
    vsg::ref_ptr<vsgGeo::OneSideRender> gridlines_;
    ObjectSet<vsg::vec4Array>	gridcolors_;
    int					nrgridsegments_[6];
    vsg::vec3				corners_[8];
    TypeSet<Tick>			ticks_;
    RefMan<Text2>			axisnametexts_;
    RefMan<Text2>			ticktexts_;

    ConstRefMan<mDGBVisTrans>	displaytrans_;
    OD::Color			annotcolor_;
    uiString			axisnames_[3];
    bool			textshown_ = true;
    bool			scaleshown_ = true;
    bool			gridshown_ = false;

    static const char*		textprefixstr();
    static const char*		cornerprefixstr();
    static const char*		showtextstr();
    static const char*		showscalestr();
};

} // namespace dGBVis
