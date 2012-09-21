#ifndef uifiledlg_h
#define uifiledlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/09/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "bufstringset.h"

class uiParent;
class FileMultiString;

/*!\brief Dialog to get a file or directory name from user
*/

mClass(uiBase) uiFileDialog
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
    enum Type		{ Gen, Img, Txt, Html };
                        uiFileDialog(uiParent*,Type,
				     const char* fname=0,
				     const char* caption=0);
						//!< Always AnyFile

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
    			// Warning starts with '!'-symbol

protected:

    BufferString	fn;
    Mode		mode_;
    BufferString	fname_;
    BufferString	filter_;
    BufferString	caption_;
    uiParent*		parnt_;
    BufferStringSet	filenames_;
    BufferString	selectedfilter_;
    BufferString	currentdir_;
    bool		addallexts_;
    bool		forread_;
    bool		confirmoverwrite_;
    
    static FileMultiString*	externalfilenames_;
    static BufferString		extfilenameserrmsg_;

private:
    int			beginCmdRecEvent( const char* wintitle );
    void		endCmdRecEvent(int refnr, bool ok);
};

#endif

