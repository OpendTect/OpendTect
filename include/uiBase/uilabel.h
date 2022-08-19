#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "draw.h"
#include "uistring.h"

class uiGroup;
class uiLabelBody;
class uiPixmap;

mExpClass(uiBase) uiLabel : public uiObject
{
public:

			uiLabel(uiParent*,const uiString&);
			uiLabel(uiParent*,const uiString&,uiObject*);
			uiLabel(uiParent*,const uiString&,uiGroup*);

/*! \brief set text on label

    Note that the layout for the label is not updated when setting a new text.
    So, if the new text is too long, part of it might be invisible.
    Therefore, reserve enough space for it with setPrefWidthInChar.

*/
    virtual void	setText(const uiString&);
    const uiString&	text() const;
    void		setTextSelectable(bool yn=true);
    void		setPixmap(const uiPixmap&);
    void		setIcon(const char* iconnm);

/*!
    setting an alignment only makes sense if you reserve space using
    setPrefWidthInChar();
*/
    void		setAlignment(Alignment::HPos);
    Alignment::HPos	alignment() const;

    void		makeRequired(bool yn=true);

private:
    void		translateText() override;

    void		init(const uiString& txt,uiObject* buddy);
    void		updateWidth();

    uiLabelBody*	body_;
    uiLabelBody&	mkbody(uiParent*,const uiString&);

    uiString		text_;

    bool		isrequired_;

};
