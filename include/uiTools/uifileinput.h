#ifndef uifileinput_h
#define uifileinput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id: uifileinput.h,v 1.1 2000-11-27 10:19:43 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiLineEdit;


class uiFileInput : public uiGroup
{ 	
public:
			uiFileInput(uiObject*,const char* txt,const char* fnm=0,
				    bool forread=true,bool withclear=false,
				    const char* filtr=0);

    const char*		fileName();

protected:

    uiLineEdit*		inpfld;
    bool		forread;
    BufferString	fname;
    BufferString	filter;

    void		doSelect(CallBacker*);
    void		doClear(CallBacker*);

};


#endif
