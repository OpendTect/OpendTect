#ifndef uifiledlg_h
#define uifiledlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.h,v 1.4 2002-03-15 15:29:20 arend Exp $
________________________________________________________________________

-*/
#include "uiobj.h"
#include <bufstring.h>

class QFileDialog;

class uiFileDialog : public UserIDObject
{
public:
    //! ToolBar Dock Identifier
    /*
        Toolbars can be created on docks,
    */
    enum Mode
    {
	AnyFile,	/*!< The name of a file, whether it exists or not. */
	ExistingFile,	/*!< The name of a single existing file. */
	Directory,	/*!< The name of a directory. Both files and
			     directories displayed. */
	DirectoryOnly,	/*!< The name of a directory. The file dialog will
			     only display directories. */
	ExistingFiles	/*!< The names of zero or more existing files. */
    };

                        uiFileDialog( uiParent* = 0, 
				      bool forread = true,
				      const char* fname = 0,
				      const char* filter = 0,
				      const char* caption = 0 );

                        uiFileDialog( uiParent* = 0, 
				      Mode mode = AnyFile,
				      const char* fname = 0,
				      const char* filter = 0,
				      const char* caption = 0 );

    const char*		fileName() const	{ return fn; }
    int                 go();

    void		setMode( Mode m)	{ mode_=m; }
    Mode		mode() const		{ return mode_; }

protected:

    BufferString	fn;
    Mode		mode_;
    BufferString	fname_;
    BufferString	filter_;
    BufferString	caption_;
};

#endif
