#ifndef uifileinput_h
#define uifileinput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id: uifileinput.h,v 1.3 2001-02-16 17:01:54 arend Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"

/*! \brief A file-name input. 

Displays a uiLineEdit field showing the current selected file. The user can
edit the filename by hand, or pop up a file selector trough the included
"Select..." push button.

*/

class uiFileInput : public uiGenInput
{ 	
public:
			uiFileInput(uiObject*,const char* txt,const char* fnm=0
			    , bool forread=true, const char* filtr=0 );

    const char*		fileName();

protected:

    bool		forread;
    BufferString	fname;
    BufferString	filter;

    virtual void	doSelect(CallBacker*);

};


#endif
