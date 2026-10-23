#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"
#include "vsggeotext.h"
#include "vsggeoupdater.h"

#include <vsg/core/Array.h>
#include <vsg/core/Inherit.h>
#include <vsg/maths/vec2.h>
#include <vsg/maths/vec4.h>
#include <vsg/nodes/Group.h>
#include <vsg/text/Font.h>

#include <functional>
#include <string>
#include <vector>

namespace vsgGeo
{

/*!\brief Counterpart of osgGeo::ScalarBar, as far as the color bar of
  OpendTect needs it: a bar of colors with a border, and a tick and a label at
  evenly spaced values, above a horizontal bar and right of a vertical one.
  For a HUD, where one unit is one pixel. */

class VSGGEO_EXPORT ScalarBar : public vsg::Inherit<vsg::Group,ScalarBar>
{
public:

    static constexpr int	cNrColors = 256;
    static constexpr int	cMaxLabels = 24;

    enum class Orientation	{ Horizontal, Vertical };

				ScalarBar();

    void			setUpdater(Updater*);

    void			setColors(const vsg::vec4*,int nrcolors);
    const vsg::vec4&		color(int idx) const;
    void			setRange(float min,float max);
    void			setNumLabels(int);
    int				getNumLabels() const	{ return nrlabels_; }
    const std::u32string&	labelText(int idx) const;
    void			setLabelFormatter(
				    const std::function<std::string(float)>&);

    void			setOrientation(Orientation);
    Orientation			orientation() const { return orientation_; }
    void			setSize(float length,float thickness);
    void			setPosition(const vsg::vec2&);
    const vsg::vec2&		getPosition() const	{ return pos_; }

    void			setAnnotationColor(const vsg::vec4&);
    void			setAnnotationFont(vsg::ref_ptr<vsg::Font>);
    void			setAnnotationTextSize(float);
    float			getAnnotationTextSize() const
				{ return textsize_; }

protected:
				~ScalarBar();

private:

    void			updateGeometry();
    void			updateColors();
    void			updateLabels();

    vsg::ref_ptr<vsg::vec3Array> vertices_;
    vsg::ref_ptr<vsg::vec4Array> vertexcolors_;
    vsg::ref_ptr<Text>		texts_[cMaxLabels];
    std::vector<vsg::vec4>	colors_;
    std::function<std::string(float)> formatter_;
    float			min_ = 0.f;
    float			max_ = 1.f;
    int				nrlabels_ = 5;
    Orientation			orientation_ = Orientation::Horizontal;
    float			length_ = 500.f;
    float			thickness_ = 20.f;
    vsg::vec2			pos_{ 0.f, 0.f };
    float			textsize_ = 18.f;
    vsg::vec4			annotcolor_{ 1.f, 1.f, 1.f, 1.f };
};

} // namespace vsgGeo
