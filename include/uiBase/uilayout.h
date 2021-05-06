#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/03/2000
________________________________________________________________________

-*/
#include "uibasemod.h"
#include "gendefs.h"

class i_LayoutItem;

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
    stretchedLeftTo, stretchedRightTo, //!< stretches widget to vertical borders
    atSamePosition
};


mExpClass(uiBase) uiConstraint
{
friend class i_LayoutItem;
public:
			uiConstraint(constraintType,i_LayoutItem* o,int marg);

    bool		operator==(const uiConstraint&) const;
    bool		operator!=(const uiConstraint&) const;

    bool		enabled() const;
    void		disable(bool yn);

protected:
    constraintType      type_;
    i_LayoutItem*       other_;
    int                 margin_;
    bool		enabled_;
};

