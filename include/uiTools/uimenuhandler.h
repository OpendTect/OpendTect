#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uiparent.h"
#include "menuhandler.h"

class uiMenu;
class uiToolBar;
/*
Implementation of MenuHandler for the dGB-based userinterface.
*/


mExpClass(uiTools) uiMenuHandler : public MenuHandler
{
public:
				uiMenuHandler(uiParent*,int id);
				~uiMenuHandler();

    uiParent*			getParent() const { return uiparent_; }

    bool			executeMenu() override;
    bool			executeMenu(int menutype,
					    const TypeSet<int>* path=0 );
				/*!<\param menutype is an integer that specifies
					   what type of menu should be
					   generated. Two numbers are reserved,
					   and the user of the class may use his
					   own codes for other circumstances.
					   The two defined values are:
					   - menutype==fromTree  menu generated
						from (a right-click on) the
						treeitem.
					   - menutype==fromScene menu generated
						from the scene.
				    \param path If menutype==fromScene the path
					   of selection (i.e. a list of the
					   ids of the paht, from scene to picked
					   object).

				*/
    int				getMenuType() const { return menutype_; }
				/*!<\returns the \a menutype specified in
					  uiMenuHandler::executeMenu.
				    \note does only give a valid
					  answer if called from a callback,
					  notified by
					  uiMenuHandler::createnotifier
					  or uiMenuHandler::handlenotifier.  */
    const TypeSet<int>*		getPath() const { return path_; }
				/*!<\returns The path of selection (i.e. a list
					   of the ids of the paht, from scene
					   to picked object). */
    const Coord3&		getPickedPos() const { return positionxyz_; }
    void			setPickedPos(const Coord3& pickedpos)
					{ positionxyz_=pickedpos; }
    const Geom::Point2D<double>& get2DPickedPos() const { return positionxy_; }
    void			set2DPickedPos(const Geom::Point2D<double>& pos)
					{ positionxy_=pos; }

    static int			fromTree();
    static int			fromScene();

protected:
    bool			executeMenuInternal();
    uiMenu*			createMenu( const ObjectSet<MenuItem>&,
					    const MenuItem* =0);
    uiParent*			uiparent_;
    int				menutype_;
    const TypeSet<int>*		path_;
    Coord3			positionxyz_;
    Geom::Point2D<double>	positionxy_;
};


mExpClass(uiTools) uiTreeItemTBHandler : public MenuHandler
{ mODTextTranslationClass(uiTreeItemTBHandler)
public:
				uiTreeItemTBHandler(uiParent*);
				~uiTreeItemTBHandler();

    void			addButtons();
    bool			executeMenu() override
				{ addButtons(); return true; }
    uiToolBar*			toolBar()	{ return tb_; }

protected:

    void			butClickCB(CallBacker*);
    void			handleEmpty();

    uiToolBar*			tb_;
    uiParent*			uiparent_;
};


mGlobal(uiTools) int	add2D3DToolButton(uiToolBar&,const char* iconnnm,
				  const uiString& menuitmtxt,
				  const CallBack& cb2d,const CallBack& cb3d,
				  int* tmid2d =nullptr,int* itmid3d =nullptr);

mGlobal(uiTools) void	add2D3DMenuItem(uiMenu&,const char* iconnnm,
				  const uiString& menuitmtxt,
				  const CallBack& cb2d,const CallBack& cb3d,
				  int* tmid2d =nullptr,int* itmid3d =nullptr);
