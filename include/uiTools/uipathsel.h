#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2020
 RCS:		$Id$
________________________________________________________________________

-*/
#include "bufstring.h"
#include "bufstringset.h"
#include "uicompoundparsel.h"
#include "uieditobjectlist.h"

mExpClass(uiTools) uiPathListEditor : public uiEditObjectList
{  mODTextTranslationClass(uiPathListEditor);
public:
    uiPathListEditor(uiParent*, const BufferStringSet&);

    void	fillList(int);
    void	editReq(bool);
    void	removeReq();
    void	itemSwitch(bool);

    BufferStringSet	paths_;
};


mExpClass(uiTools) uiPathSel : public uiCheckedCompoundParSel
{ mODTextTranslationClass(uiPathSel);
public:
    uiPathSel(uiParent*, const uiString&);
    ~uiPathSel();

    void		setPaths(const BufferStringSet&);
    uiString		getSummary() const override;
    BufferStringSet	getPaths() const { return paths_; }

    void		setEmpty();

    Notifier<uiPathSel> selChange;

protected:
    void		checkedCB(CallBacker*);
    void		doDlg(CallBacker*);
    void		initGrp(CallBacker*);

    BufferStringSet	paths_;
};
