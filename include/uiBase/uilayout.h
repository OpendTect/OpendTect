#ifndef UILAYOUT_H
#define UILAYOUT_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          03/03/2000
 RCS:           $Id: uilayout.h,v 1.3 2002-03-18 13:41:54 arend Exp $
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
    hCentered,				//!< Centers with respect to parent
    ensureLeftOf, ensureRightOf,
    ensureBelow,
    leftBorder, rightBorder, topBorder, bottomBorder,
    heightSameAs, widthSameAs,
    stretchedBelow, stretchedAbove,   //!< stretches widget to horiz. borders 
    stretchedLeftTo, stretchedRightTo //!< stretches widget to vertical borders
};


class i_uiLayoutItem;
class i_uiLayout;


#endif
