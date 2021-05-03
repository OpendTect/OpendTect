#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2020
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

    CommandDefs&	operator=(const CommandDefs&);

    void		erase() override;

    bool		addCmd(const char*, const uiString&, const char*,
			       const uiString&, const BufferStringSet&);

    uiString		getUiName(int) const;
    BufferString	getIconName(int) const;
    uiString		getToolTip(int) const;
    const uiStringSet&	getUiNames() const;

    static bool		checkCommandExists(const char*, const BufferStringSet&);
    static const CommandDefs&	getTerminalCommands(const BufferStringSet&);

protected:
    uiStringSet		uinames_;
    BufferStringSet	iconnms_;
    uiStringSet		tooltips_;

};
