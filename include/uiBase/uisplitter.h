#ifndef uisplitter_h
#define uisplitter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: uisplitter.h,v 1.6 2012-08-03 13:00:53 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

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

mClass(uiBase) uiSplitter : public uiObject
{
public:
                        uiSplitter(uiParent*,const char* nm="Splitter", 
				    bool hor=true);
			//!< Set hor to false to layout vertically

    void		addGroup(uiGroup*); //!< Group becomes my child

private:

    uiSplitterBody*	body_;
    uiSplitterBody&	mkbody(uiParent*,const char*);
};

#endif

