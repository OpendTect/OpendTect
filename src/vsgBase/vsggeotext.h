#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"
#include "vsggeoupdater.h"

#include <vsg/core/observer_ptr.h>
#include <vsg/io/Options.h>
#include <vsg/maths/mat4.h>
#include <vsg/maths/vec3.h>
#include <vsg/maths/vec4.h>
#include <vsg/nodes/Group.h>
#include <vsg/text/Font.h>
#include <vsg/text/StandardLayout.h>
#include <vsg/text/Text.h>

#include <cstdint>
#include <string>

namespace vsg { class RecordTraversal; class State; }

namespace vsgGeo
{

/*!\brief One text label, drawn with vsg::GpuLayoutTechnique.

  Setters only record the change. The Updater applies it before the next
  frame: in place when VSG allows that, otherwise by building, compiling
  and swapping in a new vsg::Text. A rebuild is needed when the font
  changes, when billboarding is switched on or off, or when the text does
  not stay strictly below the allocated number of glyphs.

  Without an Updater the change is applied at once and nothing is compiled:
  for a label that is built before the viewer exists, which the viewer then
  compiles with the rest of the scene.

  A billboard (AxisAlignment::Screen) can be screen-sized: the character
  height is then in pixels, and the size in the scene is computed from the
  distance to the camera each time the label is drawn. VSG has no such
  thing: its auto-scale distance only shrinks labels that are closer than it.

  The setters and the Updater must run on the same thread.
*/

class VSGGEO_EXPORT Text : public vsg::Inherit<vsg::Group,Text>
{
public:

    enum class HAlign		{ Left, Center, Right };
    enum class VAlign		{ Bottom, Center, Top };
    enum class AxisAlignment	{ XY, ReversedXY, XZ, ReversedXZ,
				  YZ, ReversedYZ, Screen, User };

    static constexpr uint32_t	sMinCapacity = 32;

				Text();

    void			setUpdater(Updater*);
    void			setOptions(vsg::ref_ptr<const vsg::Options>);

    void			setFont(vsg::ref_ptr<vsg::Font>);
    vsg::ref_ptr<vsg::Font>	font() const		{ return font_; }

    void			setText(const std::u32string&);
    const std::u32string&	text() const		{ return text_; }

    void			setPosition(const vsg::vec3&);
    const vsg::vec3&		position() const	{ return position_; }

    void			setCharacterHeight(float);
    float			characterHeight() const	{ return height_; }

    void			setColor(const vsg::vec4&);
    const vsg::vec4&		color() const		{ return color_; }

    void			setOutline(const vsg::vec4& color,float width);

    void			setAlignment(HAlign,VAlign);
    void			setAxisAlignment(AxisAlignment);
    AxisAlignment		axisAlignment() const	{ return axisalign_; }

    void			setRotation(float radangle,
					    const vsg::vec3& axis);
				//!<Used when the axis alignment is User

    void			setAutoScaleDistance(float);
				//!<Billboards only; 0 turns it off

    void			setScreenSized(bool);
				/*!<The character height is then in pixels.
				    Only used for billboards. */
    bool			isScreenSized() const	{ return screensized_; }
    static void			setViewportHeight(float);
				//!<Pixels; of the view the labels are drawn in
    static void			setHorizontalScale(float);
				/*!<For a view that is drawn with another shape than
				    its projection: the aspect ratio of the
				    projection divided by that of the view. The
				    labels are then not stretched with the scene.
				    Used from the next frame on. */

    void			accept(vsg::RecordTraversal&) const override;

    void			applyUpdates(vsg::Viewer&,Compiler&);

    const vsg::Text*		textNode() const	{ return current_.get(); }
    uint32_t			capacity() const	{ return capacity_; }

protected:
				~Text();

private:

    void			requestUpdate();
    void			applyChanges(Compiler*);
    void			fillLayout(vsg::StandardLayout&) const;
    vsg::ref_ptr<vsg::Data>	createTextData() const;
    vsg::ref_ptr<vsg::ShaderSet> textShaderSet();
    vsg::ref_ptr<const vsg::Options> effectiveOptions() const;
    void			rebuild(Compiler*,bool billboard);
    void			updateInPlace();
    void			updateScreenSize(const vsg::State&);

    vsg::observer_ptr<Updater>	updater_;
    vsg::ref_ptr<Updatable>	updatehelper_;
    vsg::ref_ptr<const vsg::Options> options_;

    vsg::ref_ptr<vsg::Font>	font_;
    std::u32string		text_;
    vsg::vec3			position_{ 0.f, 0.f, 0.f };
    float			height_ = 1.f;
    vsg::vec4			color_{ 1.f, 1.f, 1.f, 1.f };
    vsg::vec4			outlinecolor_{ 0.f, 0.f, 0.f, 1.f };
    float			outlinewidth_ = 0.f;
    HAlign			halign_ = HAlign::Left;
    VAlign			valign_ = VAlign::Bottom;
    AxisAlignment		axisalign_ = AxisAlignment::XY;
    vsg::mat4			rotation_;
    float			autoscaledist_ = 0.f;
    bool			screensized_ = false;
    float			screenworldheight_ = 0.f;
    float			screenhscale_ = 1.f;

    vsg::ref_ptr<vsg::Text>	current_;
    vsg::ref_ptr<vsg::ShaderSet> shaderset_;
    uint32_t			capacity_ = 0;
    bool			builtbillboard_ = false;
    bool			needsrebuild_ = true;
};

} // namespace vsgGeo
