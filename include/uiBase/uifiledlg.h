#ifndef uifiledlg_h
#define uifiledlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.h,v 1.3 2001-08-31 15:05:29 arend Exp $
________________________________________________________________________

-*/
#include "uiobj.h"
#include <bufstring.h>

class QFileDialog;

class uiFileDialog : public UserIDObject
{
public:
                        uiFileDialog( uiParent* = 0, 
				      bool forread = true,
				      const char* fname = 0,
				      const char* filter = 0,
				      const char* caption = 0 );

    const char*		fileName() const { return fn; }
    int                 go();

protected:
    BufferString	fn;
    bool		forread_;
    BufferString	fname_;
    BufferString	filter_;
    BufferString	caption_;
};

#endif

