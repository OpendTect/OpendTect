#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribparam.h"
#include "externalattrib.h"
#include "multiid.h"
#include "menuhandler.h"
#include "uioddatatreeitem.h"


namespace ExternalAttrib
{

//Class that calculates the random values

mClass(ExternalAttrib) Random : public Attrib::ExtAttribCalc
{
public:

    static const char*		sAttribName()	{ return "Random"; }

private:
				Random();
				~Random();

    bool			setTargetSelSpec(
					const Attrib::SelSpec&) override;
    ConstRefMan<RegularSeisDataPack> createAttrib(const TrcKeyZSampling&,
						  const RegularSeisDataPack*,
						  TaskRunner*) override;

    static Attrib::ExtAttribCalc* create(const Attrib::SelSpec&);

public:

    static void			initClass();
};


/* Class that manages the menus. One instance of this class resides in memory
   and makes sure that every time an object in the visualization is clicked,
   createMenu is called.
*/

mClass(ExternalAttrib) RandomManager : public CallBacker
{
public:
		RandomManager();
		~RandomManager();

protected:

    void	createMenuCB(CallBacker*);
    void	handleMenuCB(CallBacker*);

    MenuItem	addrandomattribmnuitem_;
};


/* Class that holds the external attrib's tree-item. Can easily be complemented
   with meny handling by implementing more intelligent createMenu/handleMenu. */

mClass(ExternalAttrib) uiRandomTreeItem : public uiODDataTreeItem
{
public:
    static void		initClass();
			uiRandomTreeItem(const char* parenttype);

    static const char*	sKeyDefinition() { return "Random"; }

protected:

    bool		anyButtonClick( uiTreeViewItem* );
    BufferString	createDisplayName() const;
    void		updateColumnText( int );

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,
				       const char* parenttype);
};

} // namespace ExternalAttrib
