#ifndef uilayout_h
#define uilayout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          03/03/2000
 RCS:           $Id: uilayout.h,v 1.8 2009-01-09 03:45:14 cvsnanne Exp $
________________________________________________________________________

-*/

enum constraintType 
{ 
    leftOf, rightOf, //!< LeftOf/RightOf atach widgets tightly together
    leftTo, rightTo, //!< LeftTo/RightTo allow extra horizonal distance
    leftAlignedBelow, leftAlignedAbove,
    rightAlignedBelow, rightAlignedAbove,
    alignedWith, alignedBelow, alignedAbove,	//!< Uses uiObject::horAlign()
    centeredBelow, centeredAbove,	//!< Uses i_LayoutItem::centre()
    centeredLeftOf, centeredRightOf,	//!< Uses i_LayoutItem::centre()
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
