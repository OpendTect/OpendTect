#ifndef uifileinput_h
#define uifileinput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id: uifileinput.h,v 1.2 2001-01-26 09:53:41 arend Exp $
________________________________________________________________________

-*/

#include "uigenselect.h"

class uiFileInput : public uiGenSelect
{ 	
public:
			uiFileInput(uiObject*,const char* txt,const char* fnm=0,
				    bool forread=true,bool withclear=false,
				    const char* filtr=0);

    const char*		fileName();

protected:

    bool		forread;
    BufferString	fname;
    BufferString	filter;

    virtual void	doSelect_(CallBacker*);

};


#endif
