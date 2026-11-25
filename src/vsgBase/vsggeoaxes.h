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

#include <vsg/app/Camera.h>
#include <vsg/core/Inherit.h>
#include <vsg/core/Visitor.h>
#include <vsg/core/observer_ptr.h>
#include <vsg/maths/vec2.h>
#include <vsg/maths/vec4.h>
#include <vsg/nodes/Group.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/text/Font.h>
#include <vsg/ui/PointerEvent.h>

#include <chrono>
#include <string>

namespace vsg { class RecordTraversal; }

namespace vsgGeo
{

/*!\brief Counterpart of osgGeo::AxesNode: three arrows and a ball with a label
  at each tip, turning with the primary camera. For a HUD, where one unit is
  one pixel. The arrows point north (+Y, red), east (+X, green) and down
  (-Z, blue). A change of the geometry is applied through the Updater. They
  pop slightly while the pointer is on them. */

class VSGGEO_EXPORT AxesNode : public vsg::Inherit<vsg::Group,AxesNode>
{
public:

				/*!Modern has slim flat arrows, the label
				    beyond the tip and a dot at the center */
    enum class Style		{ Classic, Modern };

				AxesNode();

    void			setStyle(Style);
    Style			style() const		{ return style_; }
				/*!Free space around the tips, in pixels */
    float			getMargin() const;

    void			setUpdater(Updater*);
				/*!Radius and length of the arrows */
    void			setSize(vsg::vec2 size);
    void			setPosition(vsg::vec2 pos);
    void			setRadius(const float&);
    float			getRadius() const	{ return radius_; }
    float			getLength() const	{ return length_; }
    void			setLength(const float&);
    void			setAnnotationColor(vsg::vec4 col);
    void			setAnnotationText(int dim,
						  const std::u32string&);
    const std::u32string&	getAnnotationText(int dim) const;
    void			setAnnotationTextSize(float);
    float			getAnnotationTextSize() const
				{ return textsize_; }
    void			setAnnotationFont(vsg::ref_ptr<vsg::Font>);
    void			setPrimaryCamera(vsg::ref_ptr<vsg::Camera>);

				/*!x and y are in pixels, y up */
    void			setPointer(float x,float y);
    void			clearPointer();
    bool			isPointerOver() const	{ return over_; }
    float			getPop() const		{ return pop_; }
    void			advance(std::chrono::steady_clock::time_point);

    void			accept(vsg::RecordTraversal&) const override;
    void			applyUpdates(vsg::Viewer&,Compiler&);

    const vsg::Node*		geometry() const
				{ return geometry_.get(); }

protected:
				~AxesNode();

private:

    void			requestUpdate();
    void			rebuild(Compiler*);
    void			applyStyleToTexts();
    float			dotRadius() const;
    float			labelDistance() const;
    void			updateTransform();

    vsg::observer_ptr<Updater>	updater_;
    vsg::ref_ptr<Updatable>	updatehelper_;
    vsg::ref_ptr<vsg::Camera>	primarycamera_;
    vsg::ref_ptr<vsg::MatrixTransform> transform_;
    vsg::ref_ptr<vsg::Group>	geometrygroup_;
    vsg::ref_ptr<vsg::Node>	geometry_;
    vsg::ref_ptr<vsg::MatrixTransform> labelxforms_[3];
    vsg::ref_ptr<vsg::MatrixTransform> centerxform_;
    vsg::ref_ptr<vsg::Group>	centergroup_;
    vsg::ref_ptr<vsg::Node>	centergeometry_;
    vsg::ref_ptr<Text>		texts_[3];
    Style			style_ = Style::Classic;
    vsg::vec4			annotcolor_{ 1.f, 1.f, 1.f, 1.f };
    float			radius_ = 1.f;
    float			length_ = 10.f;
    float			textsize_ = 18.f;
    vsg::vec2			pos_{ 0.f, 0.f };
    bool			over_ = false;
    float			pop_ = 0.f;
    std::chrono::steady_clock::time_point lastadvance_;
    bool			hasadvanced_ = false;
};


/*!\brief Gives the pointer position in the window to an AxesNode */

class VSGGEO_EXPORT AxesHandler : public vsg::Inherit<vsg::Visitor,AxesHandler>
{
public:

    void			setAxes(AxesNode*);

    void			apply(vsg::MoveEvent&) override;

private:

    vsg::observer_ptr<AxesNode>	axes_;
};

} // namespace vsgGeo
