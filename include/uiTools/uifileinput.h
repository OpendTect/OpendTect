#ifndef uifileinput_h
#define uifileinput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id: uifileinput.h,v 1.7 2001-12-06 09:36:53 nanne Exp $
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
			uiFileInput(uiParent*,const char* txt,const char* fnm=0
			    , bool forread=true, const char* filtr=0 );

    void		setFileName(const char*);
    void		setDefaultSelectionDir( const char* nm )
			{ defseldir = nm; }
    void		setFilter( const char* fltr )
			{ newfltr = true; selfltr = fltr; }
    const char*		fileName();

protected:

    bool		forread;
    bool		newfltr;
    BufferString	fname;
    BufferString	filter;
    BufferString	defseldir;
    BufferString	selfltr;

    virtual void	doSelect(CallBacker*);

};


#endif
