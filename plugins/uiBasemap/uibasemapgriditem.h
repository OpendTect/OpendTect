#ifndef uibasemapgriditem_h
#define uibasemapgriditem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		October 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"


namespace visSurvey { class PlaneDataDisplay; }
class uiCheckBox;
class uiSelLineStyle;

mExpClass(uiBasemap) uiBasemapGridGroup : public uiBasemapGroup
{
public:
			uiBasemapGridGroup(uiParent*, bool isadd);
			~uiBasemapGridGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    void		icxyCB(CallBacker*);
    void		showGridLineCB(CallBacker*);
    uiObject*		lastObject();

    uiGenInput*		icxycheck_;
    uiCheckBox*		inlxfld_;
    uiCheckBox*		crlyfld_;
    uiGenInput*		inlxspacingfld_;
    uiGenInput*		crlyspacingfld_;
    uiSelLineStyle*	lsfld_;

private:
    void		setParameters();
    void		setCheckBoxLabel();
};


mExpClass(uiBasemap) uiBasemapGridTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapGridTreeItem(const char*);
			~uiBasemapGridTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};


mExpClass(uiBasemap) uiBasemapGridItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapGridItem,
				"Grid",
				sFactoryKeyword())

    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif
