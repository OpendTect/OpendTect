#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2017
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uifileselector.h"
#include "factory.h"


/*!\brief Selects file or directory in some form of UI.
  1-1 coupled with a certain SystemAccess / protocol. */

mExpClass(uiBase) uiFileSelTool
{
public:
    virtual		~uiFileSelTool()		{}

    typedef uiFileSelectorSetup	Setup;

    uiString&		caption()		{ return caption_; }
    Setup&		setup()			{ return setup_; }

    bool		go()			{ return doSelection(); }

    BufferString	fileName() const;
    void		getSelected( BufferStringSet& bss ) const
			{ gtFileNames( bss ); }

    static BufferString	joinSelection(const BufferStringSet&);
    static void		separateSelection(const char*,BufferStringSet&);

protected:

			uiFileSelTool(uiParent*,const Setup&);

    uiParent*		parent_;
    Setup		setup_;
    uiString		caption_;

    virtual bool	doSelection()				= 0;
    virtual void	gtFileNames(BufferStringSet&) const	= 0;

};



/*!\brief provides file selector for a File::SystemAccess type */

mExpClass(uiBase) uiFileSelToolProvider
{
public:

    typedef uiFileSelectorSetup	Setup;

    static const uiFileSelToolProvider& get(const char* prot=0);

    virtual const char*		protocol() const			= 0;
    virtual uiString		userName() const;

	// To obtain a file selector for this protocol. Becomes yours.
    virtual uiFileSelTool*	getSelTool(uiParent*,const Setup&) const = 0;

    mDefineFactoryInClass( uiFileSelToolProvider, factory );

};
