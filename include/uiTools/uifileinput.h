#ifndef uifileinput_h
#define uifileinput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id: uifileinput.h,v 1.8 2002-06-11 12:25:24 arend Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uifiledlg.h"

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

    uiFileDialog::Mode	selectMode() const		
			{ 
			    return selmodset ? selmode 
					: (forread ? uiFileDialog::ExistingFile 
						   : uiFileDialog::AnyFile); 
			}
    void		setSelectMode( uiFileDialog::Mode m) 
			    { selmodset=true;  selmode= m; }

protected:

    bool		forread;
    bool		newfltr;
    BufferString	fname;
    BufferString	filter;
    BufferString	defseldir;
    BufferString	selfltr;

    bool		selmodset;
    uiFileDialog::Mode  selmode;

    virtual void	doSelect(CallBacker*);

};


#endif
