#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		June 2021
________________________________________________________________________

-*/

#include "basicmod.h"
#include "uistringset.h"
#include "bufstringset.h"


mExpClass(Basic) CommandDefs : public BufferStringSet
{
public:
			CommandDefs();
			CommandDefs(const CommandDefs& oth);
			~CommandDefs();

    CommandDefs&	operator=(const CommandDefs&);

    void		erase() override;

    bool		addCmd(const char*,const uiString&,const char*,
			       const uiString&,const BufferStringSet&);
    bool		addCmd(const char*,const uiString&,const char*,
			       const uiString&,const BufferStringSet&,
			       const BufferStringSet*);

    uiString		getUiName(int) const;
    BufferString	getIconName(int) const;
    uiString		getToolTip(int) const;
    const uiStringSet&	getUiNames() const;
    const char*		program(int) const;
    const BufferStringSet*	args(int argidx) const;

    static bool		checkCommandExists(const char*,const BufferStringSet&);
    static const CommandDefs&	getTerminalCommands(const BufferStringSet&);

protected:
    void		addApplication(const char* appnm);
    void		addApplication(const char* appnm,
				       const BufferStringSet* args);
    void		addHints(BufferStringSet&, const char*);
    BufferStringSet&	getProgNames() const;
    ObjectSet<BufferStringSet>& getProgArgs() const;

    uiStringSet		uinames_;
    BufferStringSet	iconnms_;
    uiStringSet		tooltips_;

};
