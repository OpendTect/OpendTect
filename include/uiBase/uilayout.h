#ifndef UILAYOUT_H
#define UILAYOUT_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          03/03/2000
 RCS:           $Id: uilayout.h,v 1.4 2002-03-19 12:11:24 arend Exp $
________________________________________________________________________

-*/

enum constraintType 
{ 
    leftOf, rightOf, //!< LeftOf/RightOf atach widgets tightly together
    leftTo, rightTo, //!< LeftTo/RightTo allow extra horizonal distance
    leftAlignedBelow, leftAlignedAbove,
    rightAlignedBelow, rightAlignedAbove,
    alignedBelow, alignedAbove,		//!< Uses uiObject::horAlign()
    centeredBelow, centeredAbove,	//!< Uses uiObject::horCentre()
    ensureLeftOf, ensureRightOf,
    ensureBelow,
    leftBorder, rightBorder, topBorder, bottomBorder,
    hCentered,				//!< Centers with respect to parent
    heightSameAs, widthSameAs,
    stretchedBelow, stretchedAbove,   //!< stretches widget to horiz. borders 
    stretchedLeftTo, stretchedRightTo //!< stretches widget to vertical borders
};


class i_uiLayoutItem;
class i_uiLayout;


#endif
