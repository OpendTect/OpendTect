#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistrings.h"

class uiGenInput;
class uiCheckBox;
class uiColorInput;
class uiLabeledSpinBox;
namespace OD { class LineStyle; }


/*!\brief Group for defining line properties
Provides selection of linestyle, linecolor and linewidth
*/

mExpClass(uiTools) uiSelLineStyle : public uiGroup
{ mODTextTranslationClass(uiSelLineStyle);
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup( const uiString&
			       lbltxt=uiStrings::sEmptyString() )
			    // lbltxt null or "" => "Line style"
			    // lbltxt "-" => no label
			    : txt_(lbltxt)
			    , drawstyle_(true)
			    , color_(true)
			    , width_(true)
			    , transparency_(false)
			{}


	mDefSetupMemb(uiString,txt)
	mDefSetupMemb(bool,drawstyle)
	mDefSetupMemb(bool,color)
	mDefSetupMemb(bool,width)
	mDefSetupMemb(bool,transparency)

    };

			uiSelLineStyle(uiParent*,const OD::LineStyle&,
			 const uiString& lbltxt=uiString::emptyString());
			uiSelLineStyle(uiParent*,const OD::LineStyle&,
					       const Setup&);
			~uiSelLineStyle();

    void		setStyle(const OD::LineStyle&);
    const OD::LineStyle& getStyle() const;

    void		setColor(const OD::Color&);
    const OD::Color&	getColor() const;
    void		setWidth(int);
    int			getWidth() const;
    void		setLineWidthBounds( int min, int max );
    void		setType(int);
    int			getType() const;

    Notifier<uiSelLineStyle>	changed;

protected:

    uiGenInput*			stylesel_;
    uiColorInput*		colinp_;
    uiLabeledSpinBox*		widthbox_;

    OD::LineStyle&		linestyle_;

    void			changeCB(CallBacker*);
private:

    void			init(const Setup&);

};
