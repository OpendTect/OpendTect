#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "color.h"
#include "visvsgobject.h"
#include "fontdata.h"
#include "manobjectset.h"
#include "position.h"
#include "uistring.h"
#include "vsggeotext.h"

namespace dGBVis
{

class Text2;

/*!\brief Counterpart of visBase::Text: one label, with the same interface.

  Converts the OpendTect types and forwards them to a vsgGeo::Text. The font
  comes from FontCreator: until it is loaded the label is not drawn.
  CharacterSizeMode Screen (the default) gives labels with a size in pixels,
  which needs AxisAlignment OnScreen (the default) and the viewport height in
  vsgGeo::Text::setViewportHeight. ObjectWithScreenMaximum is not implemented
  yet and behaves as Object.
*/

mExpClass(vsgBase) Text : public CallBacker
{
public:
				~Text();

    enum Justification		{ Left, Right, Center, Top, Bottom,
				  TopLeft, TopRight, BottomLeft, BottomRight };

    enum CharacterSizeMode	{ Object, Screen, ObjectWithScreenMaximum };

    enum AxisAlignment		{ XY, ReversedXY, XZ, ReversedXZ,
				  YZ, ReversedYZ, OnScreen, User };

    void			setPosition(const vsg::vec3&);
    void			setPosition(const Coord3&,
					    bool scenespace=false);
    Coord3			getPosition() const;

    void			setRotation(float radangle,const Coord3& axis);

    void			setFontData(const FontData&,float pixeldens);
    const FontData&		getFontData() const	{ return fontdata_; }

    void			updateFontSize(float pixeldensity);

    void			setText(const uiString&);
    const uiString&		getText() const		{ return text_; }

    void			setColor(const OD::Color&);
    OD::Color			getColor() const	{ return color_; }

    void			setJustification(Justification);
    int				getJustification() const
				{ return justification_; }

    vsgGeo::Text&		getVsgText()		{ return *vsgtext_; }
    const vsgGeo::Text&		getVsgText() const	{ return *vsgtext_; }

    void			setDisplayTransformation(const mDGBVisTrans*);
    void			setCharacterSizeMode(CharacterSizeMode);
    void			setAxisAlignment(AxisAlignment);

protected:
				Text();

private:

    friend class Text2;

    void			prepare();
    void			redraw();
    void			fontLoadedCB(CallBacker*);

    vsg::ref_ptr<vsgGeo::Text>	vsgtext_;
    ConstRefMan<mDGBVisTrans>	displaytrans_;
    uiString			text_;
    FontData			fontdata_;
    float			pixeldensity_;
    OD::Color			color_ = OD::Color::White();
    Justification		justification_ = BottomLeft;
    CharacterSizeMode		sizemode_ = Screen;
};


/*!\brief Counterpart of visBase::Text2: a set of labels in one object. */

mExpClass(vsgBase) Text2 : public VisualObjectImpl
{
public:
    static RefMan<Text2>	create();
				mCreateDGBVisObj(Text2);

    int				nrTexts() const		{ return texts_.size(); }
    int				addText();
    void			removeText(const Text*);
    void			removeAll();

    void			setFontData(const FontData&);

    const Text*			text(int idx=0) const;
    Text*			text(int idx=0);

    void			setDisplayTransformation(
						const mDGBVisTrans*) override;
    const mDGBVisTrans*		getDisplayTransformation() const override
				{ return displaytransform_.ptr(); }
    void			setPixelDensity(float) override;
    float			getPixelDensity() const override
				{ return pixeldensity_; }

private:
				~Text2();

    void			translationChangeCB(CallBacker*);

    ManagedObjectSet<Text>	texts_;
    float			pixeldensity_;
    ConstRefMan<mDGBVisTrans>	displaytransform_;
};

} // namespace dGBVis
