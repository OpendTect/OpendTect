#ifndef uitextedit_h
#define uitextedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.h,v 1.1 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <uiobj.h>


class uiTextEditBody;

class uiTextEdit : public uiObject
{
public:

                        uiTextEdit( uiParent* parnt, 
				    const char* nm="uiTextEdit",
				    bool readonly=false );

    void		setText( const char* );
    void		append( const char* ); 

    static int          defaultWidth()		    { return defaultWidth_; }
    static void         setDefaultWidth( int w )    { defaultWidth_ = w; }

    static int          defaultHeight()		    { return defaultHeight_; }
    static void         setDefaultHeight( int h )   { defaultHeight_ = h; }

protected:

    static int          defaultWidth_;
    static int          defaultHeight_;

private:

    uiTextEditBody*	body_;
    uiTextEditBody&	mkbody(uiParent*, const char*, bool);

};

#endif
