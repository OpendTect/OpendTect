#ifndef uifiledlg_h
#define uifiledlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.h,v 1.8 2003-03-19 16:21:59 bert Exp $
________________________________________________________________________

-*/
#include "uiobj.h"
#include <bufstring.h>

class uiFileDialog : public UserIDObject
{
public:
    //! File selection mode
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

                        uiFileDialog(uiParent*,bool forread,
				     const char* fname = 0,
				     const char* filter = 0,
				     const char* caption = 0 );

                        uiFileDialog(uiParent*,Mode mode = AnyFile,
				     const char* fname = 0,
				     const char* filter = 0,
				     const char* caption = 0 );

    const char*		fileName() const	{ return fn; }
    int                 go();

    void		setMode( Mode m)	{ mode_=m; }
    Mode		mode() const		{ return mode_; }

    void		setOkText( const char* txt )	{ oktxt_ = txt; }
    void		setCancelText( const char* txt ){ cnclxt_ = txt; }

protected:

    BufferString	fn;
    Mode		mode_;
    BufferString	fname_;
    BufferString	filter_;
    BufferString	caption_;
    BufferString	oktxt_;
    BufferString	cnclxt_;
    uiParent*		parnt_;

};

#endif
