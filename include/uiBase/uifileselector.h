#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2017
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "fileformat.h"
#include "uistring.h"
#include "oduicommon.h"
class uiParent;
class uiFileSelTool;


/*!\brief Setup for any file or directory selection process */

mExpClass(uiBase) uiFileSelectorSetup
{
public:

    typedef OD::FileSelectionMode	SelectionMode;
    typedef OD::FileContentType		ContentType;
    typedef uiFileSelectorSetup		Setup;

    mDefSetupMemb(SelectionMode,	selmode)
    mDefSetupMemb(ContentType,		contenttype)
    mDefSetupMemb(BufferStringSet,	initialselection)
    mDefSetupMemb(BufferString,		initialselectiondir)
    mDefSetupMemb(File::FormatList,	formats)
    mDefSetupMemb(bool,			allowallextensions)
    mDefSetupMemb(BufferString,		defaultextension)
    mDefSetupMemb(bool,			confirmoverwrite)
    mDefSetupMemb(bool,			onlylocal)


			uiFileSelectorSetup(const char* fnm=0);
			uiFileSelectorSetup(SelectionMode,const char* fnm=0);

    bool		isForWrite() const
			{ return !isForRead(selmode_); }
    bool		isForDirectory() const
			{ return isDirectory(selmode_); }
    Setup&		setFileName( const char* fnm )
			{
			    initialselection_.setEmpty();
			    if ( fnm && *fnm )
				initialselection_.add( fnm );
			    return *this;
			}
    Setup&		setForWrite( bool yn=true )
			{
			    selmode_ = yn ? OD::SelectFileForWrite
					  : OD::SelectFileForRead;
			    return *this;
			}
    Setup&		selectDirectory( bool yn=true )
			{
			    selmode_ = yn ? OD::SelectDirectory
					  : OD::SelectFileForRead;
			    return *this;
			}
    Setup&		selectMultiFile( bool yn=true )
			{
			    selmode_ = yn ? OD::SelectMultiFile
					  : OD::SelectFileForRead;
			    return *this;
			}
    Setup&		setInitialSelection( const char* fnm )
			{
			    initialselection_.setEmpty();
			    initialselection_.add( fnm );
			    return *this;
			}
    Setup&		setFormat( const uiString& ftype, const char* ext,
				   const char* ext2=0, const char* ext3=0 )
			{
			    formats_.setEmpty();
			    formats_.addFormat(
					File::Format(ftype,ext,ext2,ext3) );
			    return *this;
			}
    Setup&		setFormat( const File::Format& ffmt )
			{
			    formats_.setEmpty();
			    formats_.addFormat( ffmt );
			    return *this;
			}

private:

    void			init(const char*);

};


mExpClass(uiBase) uiFileSelector
{ mODTextTranslationClass(uiFileSelector);
public:

    typedef uiFileSelectorSetup	Setup;

			uiFileSelector(uiParent*,const char* fnm=0,
					bool onlylocal=false);
			uiFileSelector(uiParent*,const Setup&);
			~uiFileSelector();

    uiString&		caption()		{ return caption_; }
    Setup&		setup()			{ return setup_; }

    bool		go();

    BufferString	fileName() const;
    void		getSelected(BufferStringSet&) const;

protected:

    uiParent*		parent_;
    Setup		setup_;
    uiString		caption_;

    uiFileSelTool*	seltool_;

};
