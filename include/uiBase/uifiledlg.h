#ifndef uifiledlg_h
#define uifiledlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id: uifiledlg.h,v 1.20 2008-05-08 06:00:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"

class uiParent;
class FileMultiString;

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

    void		setDirectory( const char* dir )
			{ currentdir_ = dir; }

    void		setAllowAllExts( bool yn )	{ addallexts_ = yn; }
    void		setConfirmOverwrite( bool yn )
    			{ confirmoverwrite_ = yn; }

    int                 go();

    static const char*	filesep_;

    static void		list2String(const BufferStringSet&,
	    			    BufferString&);
    static void		string2List(const BufferString&,
	    			    BufferStringSet&);

			// To be used by cmddriver to select filename(s)
			// while closing the QFileDialog
    int			processExternalFilenames(const char* dir=0,
	    					 const char* filters=0);
    static void		setExternalFilenames(const FileMultiString&);
    static const char*	getExternalFilenamesErrMsg();

protected:

    BufferString	fn;
    Mode		mode_;
    BufferString	fname_;
    BufferString	filter_;
    BufferString	caption_;
    uiParent*		parnt_;
    BufferStringSet	filenames;
    BufferString	selectedfilter_;
    BufferString	currentdir_;
    bool		addallexts_;
    bool		forread_;
    bool		confirmoverwrite_;
    
    static FileMultiString*	externalfilenames_;
    static BufferString		extfilenameserrmsg_;
};

#endif
