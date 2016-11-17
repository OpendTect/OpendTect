#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

class uiGroup;
class uiSplitterBody;

/*! \brief Provides a splitter object

A splitter lets the user control the size of its children by dragging the
handle between them. A default splitter is vertical, putting the groups side
by side.
Example:
\code
    uiGroup* leftgrp = new uiGroup( 0, "Left Group" );
    uiGroup* rightgrp = new uiGroup( 0, "Right Group" );
    uiSplitter* splitter = new uiSplitter( this );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
\endcode

*/

mExpClass(uiBase) uiSplitter : public uiObject
{
public:

                        uiSplitter(uiParent*,const char* nm="Splitter",
				    OD::Orientation ori=OD::Vertical);
    mDeprecated		uiSplitter(uiParent*,const char*,bool qtlayoutishor);

    void		addGroup(uiGroup*); //!< Group becomes my child

private:

    uiSplitterBody*	body_;
    uiSplitterBody&	mkbody(uiParent*,const char*);

};
