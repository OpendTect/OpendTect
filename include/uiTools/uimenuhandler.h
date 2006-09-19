#ifndef uimenuhandler_h
#define uimenuhandler_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: uimenuhandler.h,v 1.3 2006-09-19 15:38:30 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "menuhandler.h"
#include "position.h"

class uiPopupMenu;
class uiMenuItem;
/*
Implementation of MenuHandler for the dGB-based userinterface.
*/


class uiMenuHandler : public MenuHandler
{
public:
    				uiMenuHandler( uiParent*, int id );

    uiParent*			getParent() const { return parent; }

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
    int				getMenuType() const { return menutype; }
    				/*!<\returns the \a menutype specified in
					  uiMenuHandler::executeMenu.
				    \note does only give a valid
				          answer if called from a callback,
					  notified by
					  uiMenuHandler::createnotifier
					  or uiMenuHandler::handlenotifier.  */
    const TypeSet<int>*		getPath() const { return path; }
    				/*!<\returns The path of selection (i.e. a list
				  	   of the ids of the paht, from scene
					   to picked object). */
    const Coord3&		getPickedPos() const { return positionxyz; }
    void			setPickedPos(const Coord3& pickedpos)
					{ positionxyz=pickedpos; }
    const Geom::Point2D<double>& get2DPickedPos() const { return positionxy; }
    void			set2DPickedPos(const Geom::Point2D<double>& pos)
					{ positionxy=pos; }
    
    static const int		fromTree;
    static const int		fromScene;

protected:
    uiPopupMenu*		createMenu( const ObjectSet<MenuItem>&,
	    				    const MenuItem* =0);
    uiParent*			parent;
    int				menutype;
    const TypeSet<int>*		path;
    Coord3			positionxyz;
    Geom::Point2D<double>	positionxy;
    				~uiMenuHandler() {}
};


#endif
