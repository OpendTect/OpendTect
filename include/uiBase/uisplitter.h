#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uiobj.h"
#include "odcommonenums.h"

class uiGroup;
class uiSplitterBody;

/*! \brief Provides a splitter object

A splitter lets the user control the size of its children by dragging the
handle between them. A default splitter lays out its children horizontally
(side by side).
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
			uiSplitter(uiParent*,const char* nm,OD::Orientation);
			~uiSplitter();
			mOD_DisableCopy(uiSplitter)

    void		addGroup(uiGroup*); //!< Group becomes my child

//			mDeprecated("Use constructor with OD::Orientation")
			uiSplitter(uiParent*,const char* nm="Splitter",
				    bool hor=true);
			//!< Set hor to false to layout vertically
private:

    uiSplitterBody*	body_;
    uiSplitterBody&	mkbody(uiParent*,const char*);
};
