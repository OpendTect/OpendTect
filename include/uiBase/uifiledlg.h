#ifndef uifiledlg_h
#define uifiledlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.h,v 1.11 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/
#include "uiobj.h"
#include "bufstringset.h"

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
    void		getFileNames(BufferStringSet&) const;

    void		setMode( Mode m)	{ mode_=m; }
    Mode		mode() const		{ return mode_; }

    void		setOkText( const char* txt )	{ oktxt_ = txt; }
    void		setCancelText( const char* txt ){ cnclxt_ = txt; }

    int                 go();

    static const char*	filesep;

    static void		list2String(const BufferStringSet&,
	    			    BufferString&);
    static void		string2List(const BufferString&,
	    			    BufferStringSet&);

protected:

    BufferString	fn;
    Mode		mode_;
    BufferString	fname_;
    BufferString	filter_;
    BufferString	caption_;
    BufferString	oktxt_;
    BufferString	cnclxt_;
    uiParent*		parnt_;
    BufferStringSet	filenames;
};

#endif
