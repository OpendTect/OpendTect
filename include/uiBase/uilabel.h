#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          7/9/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "draw.h"
#include "uistring.h"

class uiGroup;
class uiLabelBody;
class uiPixmap;


/*!\brief Text or pixmap.

    Note that the layout for the label is not updated when setting a new text.
    So, if the new text is too long, part of it might be invisible.
    Therefore, reserve enough space for it with setPrefWidthInChar.

*/

mExpClass(uiBase) uiLabel : public uiObject
{
public:

			uiLabel(uiParent*,const uiString&);
			uiLabel(uiParent*,const uiString&,uiObject*);
			uiLabel(uiParent*,const uiString&,uiGroup*);

    virtual void	setText(const uiString&);
    const uiString&	text() const;
    void		setTextSelectable(bool yn=true);
    void		setPixmap(const uiPixmap&);
    void		setIcon(const char* iconnm);

			//! setting an alignment only makes sense if you reserve
			//! space using setPrefWidthInChar()
    void		setAlignment(OD::Alignment::HPos);
    OD::Alignment::HPos alignment() const;

    void		makeRequired(bool yn=true);

private:

    void		translateText();
    void		init(const uiString& txt,uiObject* buddy);

    uiLabelBody*	body_;
    uiLabelBody&	mkbody(uiParent*,const uiString&);

    uiString		text_;
    OD::Alignment::HPos horalign_;
    bool		isrequired_;

};
