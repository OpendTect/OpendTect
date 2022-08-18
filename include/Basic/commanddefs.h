#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    bool		addCmd(const char* appnm,const uiString& uinm,
			       const char* iconnm,const uiString& tooltip,
			       const BufferStringSet& paths,
			       const BufferStringSet* cmdargs =nullptr);

    const char*		program(int) const;
    const BufferStringSet* args(int) const;
    uiString		getUiName(int) const;
    BufferString	getIconName(int) const;
    uiString		getToolTip(int) const;
    const uiStringSet&	getUiNames() const;

    static const CommandDefs&	getTerminalCommands(const BufferStringSet&);

protected:
    static bool		checkCommandExists(const char*,const BufferStringSet&);
    void		addApplication(const char* appnm,
				       const BufferStringSet* args);
    void		addHints(BufferStringSet&, const char*);

    BufferStringSet	prognames_;
    ObjectSet<BufferStringSet> progargs_;
    uiStringSet		uinames_;
    BufferStringSet	iconnms_;
    uiStringSet		tooltips_;

};
