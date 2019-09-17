#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiComboBox;
class uiCheckBox;
class uiColorInput;
class uiLabeledSpinBox;
namespace OD { class LineStyle; };


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
			       lbltxt=uiString::empty() )
			    // lbltxt null or "" => "Line style"
			    // lbltxt "-" => no label
			    : txt_(lbltxt)
			    , drawstyle_(true)
			    , color_(true)
			    , width_(true)
			    , transparency_(false)
			    , withcheck_(true)
			{}

	mDefSetupMemb(uiString,txt)
	mDefSetupMemb(bool,drawstyle)
	mDefSetupMemb(bool,color)
	mDefSetupMemb(bool,width)
	mDefSetupMemb(bool,transparency)
	mDefSetupMemb(bool,withcheck)

    };

			uiSelLineStyle(uiParent*,const OD::LineStyle&,
			 const uiString& lbltxt=uiString::empty());
			uiSelLineStyle(uiParent*,const OD::LineStyle&,
					       const Setup&);
			~uiSelLineStyle();

    void		setStyle(const OD::LineStyle&);
    const OD::LineStyle&	getStyle() const;

    void		setColor(const Color&);
    const Color&	getColor() const;
    bool		doDraw() const;
    void		setDoDraw(bool);
    void		setWidth(int);
    int			getWidth() const;
    void		setLineWidthBounds( int min, int max );
    void		setType(int);
    int			getType() const;

    Notifier<uiSelLineStyle>	changed;

protected:

    uiComboBox*			stylesel_;
    uiColorInput*		colinp_;
    uiLabeledSpinBox*		widthbox_;
    uiCheckBox*			needline_;

    OD::LineStyle&		linestyle_;

    void			changeCB(CallBacker*);
    void			needlineCB(CallBacker*);
private:

    void			init(const Setup&);

};
