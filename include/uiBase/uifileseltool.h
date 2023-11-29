#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    virtual		~uiFileSelTool();
			mOD_DisableCopy(uiFileSelTool)

    uiString&		caption()		{ return caption_; }
    uiFileSelectorSetup& setup()		{ return setup_; }

    bool		go()			{ return doSelection(); }

    BufferString	fileName() const;
    void		getSelected( BufferStringSet& bss ) const
			{ gtFileNames( bss ); }

    static BufferString joinSelection(const BufferStringSet&);
    static void		separateSelection(const char*,BufferStringSet&);

    virtual IOPar*	getPars() const			{ return nullptr; }

protected:

			uiFileSelTool(uiParent*,const uiFileSelectorSetup&);

    uiParent*		parent_;
    uiFileSelectorSetup setup_;
    uiString		caption_;

    virtual bool	doSelection()				= 0;
    virtual void	gtFileNames(BufferStringSet&) const	= 0;

};



/*!\brief provides file selector for a File::SystemAccess type */

mExpClass(uiBase) uiFileSelToolProvider
{
public:
				mOD_DisableCopy(uiFileSelToolProvider)

    typedef uiFileSelectorSetup Setup;

    static const uiFileSelToolProvider& get(const char* prot=0);

    virtual const char*		protocol() const			= 0;
    virtual uiString		userName() const;

	// To obtain a file selector for this protocol. Becomes yours.
    virtual uiFileSelTool*	getSelTool(uiParent*,const Setup&) const = 0;

    mDefineFactoryInClass( uiFileSelToolProvider, factory );

    static void			addPluginFileSelProviders();

protected:
				uiFileSelToolProvider();
};
