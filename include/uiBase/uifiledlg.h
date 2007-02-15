#ifndef uifiledlg_h
#define uifiledlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.h,v 1.16 2007-02-15 18:50:23 cvsnanne Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"

class uiParent;

/*!\brief Dialog to get a file or directory name from user
*/

class uiFileDialog
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
				     const char* fname=0,
				     const char* filter=0,
				     const char* caption=0);

                        uiFileDialog(uiParent*,Mode mode=AnyFile,
				     const char* fname=0,
				     const char* filter=0,
				     const char* caption=0);

    const char*		fileName() const	{ return fn; }
    void		getFileNames(BufferStringSet&) const;

    void		setMode( Mode m)	{ mode_=m; }
    Mode		mode() const		{ return mode_; }

    void		setSelectedFilter(const char* fltr)
    			{ selectedfilter_ = fltr; }
    const char*		selectedFilter() const	{ return selectedfilter_; }

    void		setOkText( const char* txt )	{ oktxt_ = txt; }
    void		setCancelText( const char* txt ){ cnclxt_ = txt; }

    void		setAllowAllExts( bool yn=true )	{ addallexts_ = yn; }

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
    BufferString	selectedfilter_;
    bool		addallexts_;
    bool		forread_;
};

#endif
