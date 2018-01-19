#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddatatreeitem.h"
#include "datapack.h"

class AttribProbeLayer;

/*! Implementation of uiODDataTreeItem for standard attribute displays. */

mExpClass(uiODMain) uiODAttribTreeItem : public uiODDataTreeItem
{ mODTextTranslationClass(mODTextTranslationClass);
public:
			uiODAttribTreeItem( const char* parenttype );
			~uiODAttribTreeItem();

    const AttribProbeLayer* attribProbeLayer() const;
    AttribProbeLayer*	attribProbeLayer();

    void		prepareForShutdown();
    void		setProbeLayer(ProbeLayer*);
    static uiString	createDisplayName( int visid, int attrib );
    static uiString	sKeySelAttribMenuTxt();
    static uiString	sKeyColSettingsMenuTxt();
    static uiString	sKeyUseColSettingsMenuTxt();
    virtual void	updateDisplay();

protected:

    bool		anyButtonClick(uiTreeViewItem*);
    void		keyPressCB(CallBacker*);

    virtual bool	init();
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		updateColumnText(int col);
    uiString		createDisplayName() const;
    void		createSelMenu(MenuItem&);
    bool		handleSelMenu(int mnuid);
    ConstRefMan<DataPack> calculateAttribute();
    virtual DataPackMgr& getDPM();
    virtual void	colSeqChg(const ColTab::Sequence&);

    MenuItem		selattrmnuitem_;
    MenuItem		colsettingsmnuitem_;
    MenuItem		usecolsettingsmnuitem_;
};
