#ifndef uisplitter_h
#define uisplitter_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: uisplitter.h,v 1.1 2007-05-04 05:49:35 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiGroup;
class uiSplitterBody;

class uiSplitter : public uiObject
{
public:

                        uiSplitter(uiParent*,const char* nm="Splitter", 
				    bool hor=true);

    void		addObject(uiObject*);
    void		addGroup(uiGroup*);

private:

    uiSplitterBody*	body_;
    uiSplitterBody&	mkbody(uiParent*,const char*);
};

#endif
