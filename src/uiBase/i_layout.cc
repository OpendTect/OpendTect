/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.cc,v 1.49 2002-03-19 12:11:24 arend Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
#include "errh.h"

#include "i_layout.h"
#include "i_layoutitem.h"
#include "uiobjbody.h"
#include "uimainwin.h"

#include <qptrlist.h>
#include <qmenubar.h>

#include <stdio.h>
#include <iostream>
#include <limits.h>

#ifdef __debug__
#define MAX_ITER	2000
#else
#define MAX_ITER	10000
#endif


//------------------------------------------------------------------------------

i_LayoutItem::i_LayoutItem( i_LayoutMngr& m, QLayoutItem& itm ) 
    : mngr_( m ), mQLayoutItem_( itm ) 
    , preferred_pos_inited( false ), minimum_pos_inited( false )
    , prefSzDone( false ), hsameas( false ), vsameas( false )
{
    constrList.setAutoDelete(true); 
}


i_LayoutItem::~i_LayoutItem()
{
    delete &mQLayoutItem_;
    constrList.clear();
}


void i_LayoutItem::invalidate() 
{ 
    mQLayoutItem_.invalidate();
    preferred_pos_inited = false;
}


uiSize i_LayoutItem::actualSize( bool include_border ) const
{ 
    return curpos(setGeom).pixelSize(); 
}


int i_LayoutItem::stretch( bool hor ) const
{ 
    if ( hor && hsameas || !hor && vsameas ) return 0;

    const uiObjectBody* blo = bodyLayouted();
    return blo ? blo->stretch( hor ) : 0; 
}


void i_LayoutItem::commitGeometrySet( bool isPrefSz )
{
    uiRect mPos = curpos( setGeom );

    if ( isPrefSz ) curpos( preferred ) = mPos;

    if ( objLayouted() ) objLayouted()->triggerSetGeometry( this, mPos );
#if 0
    cout << "setting Layout on: ";
    if( objLayouted() ) 
	cout << objLayouted()->name() << endl;
    else 
	cout << "Unknown" << endl;

    cout << "l: " << mPos.left() << " t: " << mPos.top();
    cout << " hor: " << mPos.hNrPics() << " ver: " << mPos.vNrPics() << endl;
#endif
    mQLayoutItem_.setGeometry ( QRect ( mPos.left(), mPos.top(), 
                                        mPos.hNrPics(), mPos.vNrPics() )); 
}

void i_LayoutItem::initLayout( layoutMode lom, int mngrTop, int mngrLeft )
{
    uiRect& mPos = curpos( lom );
    int pref_h_nr_pics =0;
    int pref_v_nr_pics =0;


    if ( lom != minimum )
    {
	if ( bodyLayouted() )
	{
	    pref_h_nr_pics	= bodyLayouted()->prefHNrPics();
	    pref_v_nr_pics	= bodyLayouted()->prefVNrPics();
	}
	else
	{
	    pref_h_nr_pics	= prefSize().width();
	    pref_v_nr_pics	= prefSize().height();
	}
    }

    switch ( lom )
    {
	case minimum:
            if ( !minimum_pos_inited)
	    {
		mPos.zero();
		mPos.setHNrPics( minimumSize().width() );
		mPos.setVNrPics( minimumSize().height() );
		minimum_pos_inited = true;
	    }
	    break;

	case setGeom:
	    {
		uiRect& pPos = curpos(preferred);
		if ( !preferred_pos_inited )
		{
		    pPos.setLeft( 0 );
		    pPos.setTop( 0 );

		    pPos.setHNrPics( pref_h_nr_pics  );
		    pPos.setVNrPics( pref_v_nr_pics );
		    preferred_pos_inited = true;
		}
		uiRect& mPos = curpos( lom );
		mPos = curpos( preferred );

		mPos.leftTo( mMAX( pPos.left(), mngrLeft ));
		mPos.topTo( mMAX( pPos.top(), mngrTop ));

		initChildLayout(lom);
	    }
	    break;

	case preferred:
	    {
		mPos.setLeft( mngrLeft );
		mPos.setTop( mngrTop );

		mPos.setHNrPics( pref_h_nr_pics  );
		mPos.setVNrPics( pref_v_nr_pics );
		preferred_pos_inited = true;
	    }
	    break;
    } 

    if ( mPos.left() < 0 ) 
	{ pErrMsg("left < 0"); }
    if ( mPos.top() < 0 ) 
	{ pErrMsg("top < 0"); }

}


#ifdef __debug__

int i_LayoutItem::isPosOk( uiConstraint* c, int i, bool chkNrIters )
{
    if ( chkNrIters )	{ if ( i < MAX_ITER )	return i; }
    else		{ if ( i <= 2000 )	return i; }

    if ( c->enabled() ) 
    {
	BufferString msg;
	if ( chkNrIters )
	    msg = "\n  Too many iterations with: \"";
	else
	    msg = "\n  Layout loop on: \"";
	msg+= objLayouted() ? (const char*)objLayouted()->name() : "UNKNOWN";
	msg+= "\"";

	switch ( c->type )
	{
	    case leftOf: 		msg+= " leftOf "; break;
	    case rightOf:		msg+= " rightOf "; break;
	    case leftTo:		msg+= " leftTo "; break;
	    case rightTo:		msg+= " rightTo "; break;
	    case leftAlignedBelow:	msg+= " leftAlignedBelow "; break;
	    case leftAlignedAbove:	msg+= " leftAlignedAbove "; break;
	    case rightAlignedBelow:	msg+= " rightAlignedBelow "; break;
	    case rightAlignedAbove:	msg+= " rightAlignedAbove "; break;
	    case alignedBelow:		msg+= " alignedBelow "; break;
	    case alignedAbove:		msg+= " alignedAbove "; break;
	    case centeredBelow:		msg+= " centeredBelow "; break;
	    case centeredAbove:		msg+= " centeredAbove "; break;
	    case ensureLeftOf:		msg+= " ensureLeftOf "; break;
	    case ensureRightOf:		msg+= " ensureRightOf "; break;
	    case ensureBelow:		msg+= " ensureBelow "; break;
	    case leftBorder:		msg+= " leftBorder "; break;
	    case rightBorder:		msg+= " rightBorder "; break;
	    case topBorder:		msg+= " topBorder "; break;
	    case bottomBorder:		msg+= " bottomBorder "; break;
	    case heightSameAs: 		msg+= " heightSameAs "; break;
	    case widthSameAs:		msg+= " widthSameAs "; break;
	    case stretchedBelow:	msg+= " stretchedBelow "; break;
	    case stretchedAbove:	msg+= " stretchedAbove "; break;
	    case stretchedLeftTo:	msg+= " stretchedLeftTo "; break;
	    case stretchedRightTo:	msg+= " stretchedRightTo "; break;
	    default:		 	msg+= " .. "; break;
	}

	msg+= "\"";
	msg+= c->other && c->other->objLayouted() ? 
		    (const char*)c->other->objLayouted()->name() : "UNKNOWN";
	msg+= "\"";
	pErrMsg( msg );

	c->disable();
    }
    return i;
}


#define mCP(val)	isPosOk(constr,(val),false)
#define mUpdated()	{ isPosOk(constr,iternr,true); updated=true; }

#else

#define mCP(val)	(val)
#define mUpdated()	{ updated=true; }

#endif


#define mHorSpacing (constr->margin >= 0 ? constr->margin : mngr_.horSpacing())
#define mVerSpacing (constr->margin >= 0 ? constr->margin : mngr_.verSpacing())

#define mFullStretch() (constr->margin < -1)
#define mInsideBorder  (constr->margin > mngr_.borderSpace() \
			 ? constr->margin - mngr_.borderSpace() : 0)

bool i_LayoutItem::layout( layoutMode lom, const int iternr, bool finalLoop )
{
    bool updated = false;
    uiRect& mPos = curpos(lom);

    for ( constraintIterator it(constrList);
				uiConstraint* constr = it.current(); ++it )
    {
	const uiRect& otherPos 
		= constr->other ? constr->other->curpos(lom) : curpos(lom);

	switch ( constr->type )
	{
	case rightOf:
	case rightTo:
	{
	    if ( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated(); 

	    if ( mPos.topToAtLeast( mCP(otherPos.top()) ) ) 
		 mUpdated();

	    break;
	}
	case leftOf:  
	{
	    if ( mPos.rightToAtLeast(mCP(otherPos.left() - mHorSpacing)))
		mUpdated(); 

	    if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		 mUpdated();

	    break;
	}
	case leftTo:  
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		 mUpdated();

	    break;
	}
	case leftAlignedBelow:
	{
	    if ( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated(); 

	    if ( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		mUpdated();

	    break;
	}
	case leftAlignedAbove: 
	{
	    if ( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		mUpdated();

	    break;
	}
	case rightAlignedBelow:
	{
	    if ( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    if ( mPos.rightToAtLeast( mCP(otherPos.right())) )
		mUpdated();

	    break;
	}
	case rightAlignedAbove: 
	{
	    if ( mPos.rightToAtLeast( mCP(otherPos.right()) ) )
		mUpdated();

	    break;
	}
	case alignedBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    int malign = horAlign( lom );
	    int othalign = constr->other->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) ) 
		mUpdated();

	    break;
	}
	case alignedAbove:
	{ 
	    int malign = horAlign( lom );
	    int othalign = constr->other->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) ) 
		mUpdated();

	    break;
	}
	case centeredBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated(); 

	    if ( horCentre(lom) > 0 && constr->other->horCentre(lom) > 0 &&
		mPos.leftToAtLeast( mCP(mPos.left() 
				    + constr->other->horCentre(lom) 
				    - horCentre(lom)) 
				  )
	      ) 
		mUpdated();
	    break;
	}
	case centeredAbove: 
	{
	    if ( horCentre(lom) > 0 && constr->other->horCentre(lom) > 0 &&
		mPos.leftToAtLeast( mCP(mPos.left() 
				    + constr->other->horCentre(lom) 
				    - horCentre(lom)) 
				  )
	      ) 
		mUpdated();
	    break;
	} 


	case hCentered: 
	{
	    if ( finalLoop )
	    {
		int mngrcentre = ( mngr().curpos(lom).left()
				     + mngr().curpos(lom).right() ) / 2;

		int shift = mngrcentre >= 0 ?  mngrcentre - horCentre(lom) : 0;

		if ( shift > 0 )
		{
		    if ( mPos.leftToAtLeast( mCP(mPos.left() + shift) ) ) 
			mUpdated();
		}
	    }
	    break;
	} 


	case ensureRightOf:
	{
	    if ( mPos.leftToAtLeast( mCP(otherPos.right() + mHorSpacing )))  
		mUpdated(); 

	    break;
	}
	case ensureBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing ))) 
		mUpdated(); 

	    break;
	}
	case leftBorder:
	{
	    if ( finalLoop )
	    {
		int nwLeft = mngr().curpos(lom).left() + mInsideBorder;
		if ( mPos.left() != nwLeft )
		{
		    mPos.leftTo( mCP(nwLeft));
		    mUpdated();
		}
	    }
	    break;
	}
	case rightBorder:
	{
	    if ( finalLoop )
	    {
		int nwRight = mngr().curpos(lom).right() - mInsideBorder;
		if ( mPos.right() != nwRight )
		{
		    mPos.rightTo( mCP(nwRight));
		    mUpdated();
		}
	    }
	    break;
	}
	case topBorder:
	{
	    if ( finalLoop )
	    {
		int nwTop = mngr().curpos(lom).top() + mInsideBorder;
		if ( mPos.top() != nwTop )
		{
		    mPos.topTo( mCP(nwTop ));
		    mUpdated();
		}
	    }
	    break;
	}
	case bottomBorder:
	{
	    if ( finalLoop )
	    {
		int nwBottom = mngr().curpos(lom).bottom()- mInsideBorder;
		if ( mPos.bottom() != nwBottom )
		{
		    mPos.bottomTo( mCP(nwBottom ));
		    mUpdated();
		}
	    }
	    break;
	}
	case heightSameAs:
	{
	    if ( mPos.height() < ( otherPos.height() ) )
	    {
		mPos.setHeight( otherPos.height() );
		mUpdated();
	    }
	    break;
	}
	case widthSameAs:
	{
	    if ( mPos.width() < ( otherPos.width() ) )
	    {
		mPos.setWidth( otherPos.width() );
		mUpdated();
	    }
	    break;
	}
	case stretchedBelow:
	{
	    int nwLeft = mFullStretch() ? mngr().winpos(lom).left() 
					: mngr().curpos(lom).left();

	    if ( finalLoop && mPos.left() != nwLeft )
	    {
		mPos.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).width() 
					: mngr().curpos(lom).width();

	    if ( finalLoop &&  mPos.width() < nwWidth )
	    {
		mPos.setWidth( nwWidth );
		mUpdated();
	    }
	    if ( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated(); 

	    break;
	}
	case stretchedAbove:
	{
	    int nwLeft = mFullStretch() ? mngr().winpos(lom).left() 
					: mngr().curpos(lom).left();
	    if ( finalLoop && mPos.left() != nwLeft )
	    {
		mPos.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).width() 
					: mngr().curpos(lom).width();

	    if ( finalLoop &&  mPos.width() < nwWidth )
	    {
		mPos.setWidth( nwWidth );
		mUpdated();
	    }

	    break;
	}
	case stretchedLeftTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top() 
					: mngr().curpos(lom).top();
	    if ( finalLoop && mPos.top() != nwTop )
	    {
		mPos.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).height() 
					  : mngr().curpos(lom).height();
	    if ( finalLoop && mPos.height() < nwHeight )
	    {
		mPos.setHeight( nwHeight );
		mUpdated();
	    }

	    break;
	}
	case stretchedRightTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top() 
					: mngr().curpos(lom).top();
	    if ( finalLoop && mPos.top() != nwTop )
	    {
		mPos.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).height() 
					  : mngr().curpos(lom).height();
	    if ( finalLoop && mPos.height() < nwHeight )
	    {
		mPos.setHeight( nwHeight );
		mUpdated();
	    }
	    if ( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated(); 

	    break;
	}
	case ensureLeftOf:
	{
	    break;
	}
	default:
	{
	    pErrMsg("Unknown constraint type");
	    break;
	}
	}
    }

    return updated;
}


void i_LayoutItem::attach ( constraintType type, i_LayoutItem *other, 
			    int margn )
{
    if ( type != ensureLeftOf)
	constrList.append( new uiConstraint( type, other, margn ) );

    switch ( type )
    {
    case leftOf:
	other-> constrList.append(new uiConstraint( rightOf, this, margn ));
    break;

    case rightOf:
	other-> constrList.append(new uiConstraint( leftOf, this, margn ));
    break;

    case leftTo:
	other-> constrList.append(new uiConstraint( rightTo, this, margn));
    break;

    case rightTo:
	other-> constrList.append(new uiConstraint( leftTo, this, margn ));
    break;

    case leftAlignedBelow:
	other-> constrList.append( 
			new uiConstraint( leftAlignedAbove, this, margn ));
    break;

    case leftAlignedAbove:
	other-> constrList.append( 
			new uiConstraint( leftAlignedBelow, this, margn ));
    break;

    case rightAlignedBelow:
	other-> constrList.append( 
			new uiConstraint( rightAlignedAbove, this, margn ));
    break;

    case rightAlignedAbove:
	other-> constrList.append( 
			new uiConstraint( rightAlignedBelow, this, margn ));
    break;

    case alignedBelow:
	other-> constrList.append( 
			new uiConstraint( alignedAbove, this, margn ));
    break;

    case alignedAbove:
	other-> constrList.append( 
			new uiConstraint( alignedBelow, this, margn ));
    break;

    case centeredBelow:
	other-> constrList.append( 
			new uiConstraint( centeredAbove, this, margn ));
    break;

    case centeredAbove:
	other-> constrList.append( 
			new uiConstraint( centeredBelow, this, margn ));
    break;

    case heightSameAs:
	vsameas=true;
    break;

    case widthSameAs:
	hsameas=true;
    break;

    case ensureLeftOf:
	other-> constrList.append( 
			new uiConstraint( ensureRightOf, this, margn ));
    break;

    case stretchedBelow:
    break;

    case stretchedAbove:
	other-> constrList.append( 
			new uiConstraint( ensureBelow, this, margn ));
    break;

    case stretchedLeftTo:
	other-> constrList.append( 
			new uiConstraint( stretchedRightTo, this, margn ));
    break;

    case stretchedRightTo:
	other-> constrList.append( 
			new uiConstraint( stretchedLeftTo, this, margn ));
    break;

    case leftBorder:
    case rightBorder:
    case topBorder:
    case bottomBorder:
    case ensureRightOf:
    case ensureBelow:
    case hCentered:
    break;

    default:
	pErrMsg("Unknown constraint type");
    break;
    }
}


#ifdef __debug__
bool i_LayoutItem::isAligned() const
{
    for ( constraintIterator it(constrList); 
				uiConstraint* constr = it.current(); ++it )
    {
	if ( constr->type >= alignedBelow && constr->type <= centeredAbove )
	    return true; 
    }

    return false;
}
#endif


//------------------------------------------------------------------

class i_LayoutIterator : public QGLayoutIterator
{
public:
    			i_LayoutIterator( QPtrList<i_LayoutItem> *l ) 
			    : idx(0), list(l)	{}

    uint		count() 		const;
    QLayoutItem*	current();
    QLayoutItem*	next();
    QLayoutItem*	takeCurrent();
    i_LayoutItem*	takeCurrent_();

private:

    int			idx;

    QPtrList<i_LayoutItem>* list;
};


uint i_LayoutIterator::count() const
{
    return list->count();
}


QLayoutItem* i_LayoutIterator::current()
{
    return idx < int(count()) ? &(list->at( idx )->mQLayoutItem()) : 0;
}


QLayoutItem* i_LayoutIterator::next()
{
    idx++; return current();
}


QLayoutItem* i_LayoutIterator::takeCurrent()
{
    return idx < int(count()) ? &(list->take( idx )->mQLayoutItem()) : 0;
}


i_LayoutItem* i_LayoutIterator::takeCurrent_()
{
    return idx < int(count()) ? (list->take( idx )) : 0;
}

//-----------------------------------------------------------------


i_LayoutMngr::i_LayoutMngr( QWidget* parnt, 
			    const char *name, uiObjectBody& mngbdy )
    : QLayout(parnt,0,0,name), UserIDObject(name), prevGeometry()
    , minimumDone(false), preferredDone(false), ismain(false )
    , managedBody(mngbdy), hspacing(-1), vspacing(8), borderspc(0)
{ 
    childrenList.setAutoDelete(true); 
}


i_LayoutMngr::~i_LayoutMngr()
{
    childrenList.clear();
}


void i_LayoutMngr::addItem( i_LayoutItem* itm )
{
    if ( !itm ) return;
    childrenList.append( itm );
}


/*! \brief Adds a QlayoutItem to the manager's children

    Should normally not been called, since all ui items are added to the
    parent's manager using i_LayoutMngr::addItem( i_LayoutItem* itm )

*/
void i_LayoutMngr::addItem( QLayoutItem *qItem )
{
    if ( !qItem ) return;
    childrenList.append( new i_LayoutItem( *this, *qItem) );
}


QSize i_LayoutMngr::minimumSize() const
{
    if ( !minimumDone ) 
    { 
	doLayout( minimum, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->minimumDone=true; 
    }

    uiRect mPos;

    if ( ismain )
    {
	if ( managedBody.shrinkAllowed() )	
	    return QSize(0, 0);

	QSize sh = sizeHint();

	int hsz = sh.width();
	int vsz = sh.height();

	if ( hsz <= 0 || hsz > 4096 || vsz <= 0 || vsz > 4096 )
	{
	    mPos = curpos(minimum);
	    hsz = mPos.hNrPics();
	    vsz = mPos.vNrPics();
	}

	return QSize( hsz, vsz );
    }

    mPos = curpos(minimum);
    return QSize( mPos.hNrPics(), mPos.vNrPics() );
}


QSize i_LayoutMngr::sizeHint() const
{
    if ( !preferredDone )
    { 
	doLayout( preferred, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->preferredDone=true; 
    }
    uiRect mPos = curpos(preferred);

#ifdef __debug__
    if ( mPos.hNrPics() > 4096 || mPos.vNrPics() > 4096 )
    {
	BufferString msg;
	msg="Very large preferred size for ";
	msg += UserIDObject::name();
	msg += ". (h,v)=(";
	msg += mPos.hNrPics();
	msg +=" , ";
	msg += mPos.vNrPics();
	msg += ").";

	msg += " (t,l),(r,b)=(";
	msg += mPos.top();
	msg +=" , ";
	msg += mPos.left();
	msg += "),(";
	msg += mPos.right();
	msg +=" , ";
	msg += mPos.bottom();
	msg += ").";

	pErrMsg(msg);
    }
#endif

    return QSize( mPos.hNrPics(), mPos.vNrPics() );
}


QSizePolicy::ExpandData i_LayoutMngr::expanding() const
{
    return QSizePolicy::BothDirections;
}


const uiRect& i_LayoutMngr::curpos(layoutMode lom) const 
{ 
    i_LayoutItem* managedItem = 
	    const_cast<i_LayoutItem*>(managedBody.layoutItem());

    return managedItem ? managedItem->curpos(lom) : layoutpos[lom]; 
}


uiRect& i_LayoutMngr::curpos(layoutMode lom)
{ 
    i_LayoutItem* managedItem = 
	    const_cast<i_LayoutItem*>(managedBody.layoutItem());

    return managedItem ? managedItem->curpos(lom) : layoutpos[lom]; 
}


uiRect i_LayoutMngr::winpos( layoutMode lom ) const 
{
    i_LayoutItem* managedItem = 
	    const_cast<i_LayoutItem*>(managedBody.layoutItem());

    if ( ismain && !managedItem )
    {
	int hborder = layoutpos[lom].left();
	int vborder = layoutpos[lom].top();
	return uiRect( 0, 0, layoutpos[lom].right()+2*hborder,
			     layoutpos[lom].bottom()+2*vborder );
    }

    if ( ismain ) { return managedItem->curpos(lom); }

    return managedItem->mngr().winpos(lom);
}


QLayoutIterator i_LayoutMngr::iterator()
{
    return QLayoutIterator( new i_LayoutIterator( &childrenList ) ) ;
}


//! \internal class used when resizing a window
class resizeItem
{
#define NR_RES_IT	3
public:
			resizeItem( i_LayoutItem* it, int hStre, int vStre ) 
			    : item( it ), hStr( hStre ), vStr( vStre )
			    , hDelta( 0 ), vDelta( 0 ) 
			    , nhiter( hStre ? NR_RES_IT : 0 )
			    , nviter( vStre ? NR_RES_IT : 0 ) {}

    i_LayoutItem* 	item;
    const int 		hStr;
    const int 		vStr;
    int			nhiter;
    int			nviter;
    int			hDelta;
    int			vDelta;

};


void i_LayoutMngr::childrenClear( uiObject* cb )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObject* cl = curChld->objLayouted();
	if ( cl && cl != cb ) cl->clear();
    }
}


bool i_LayoutMngr::isChild( uiObject* obj )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObject* cl = curChld->objLayouted();
	if ( cl && cl == obj ) return true;
    }
    return false;
}

int i_LayoutMngr::childStretch( bool hor ) const
{

    int sum=0;
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObjectBody* ccbl = curChld->bodyLayouted();
	if ( ccbl ) sum = mMAX( sum, ccbl->stretch( hor ) );
    }

    return sum;
}

int i_LayoutMngr::horSpacing() const
{
    return hspacing >= 0 ? hspacing :  managedBody.fontWdt();
}


void i_LayoutMngr::forceChildrenRedraw( uiObjectBody* cb, bool deep )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiObjectBody* cl = curChld->bodyLayouted();
	if ( cl && cl != cb ) cl->reDraw( deep );
    }

}

void i_LayoutMngr::fillResizeList( ObjectSet<resizeItem>& resizeList, 
				   bool isPrefSz )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	int hs = curChld->stretch(true);
	int vs = curChld->stretch(false);

	if ( hs || vs )
        {
	    bool add=false;

	    if ( (hs>1) || (hs==1 && !isPrefSz) )	add = true;
	    else					hs=0;

	    if ( (vs>1) || (vs==1 && !isPrefSz) )	add = true;
	    else					vs=0;

	    if ( add ) resizeList += new resizeItem( curChld, hs, vs );
        }
    } 
}


void i_LayoutMngr::moveChildrenTo(int rTop, int rLeft, layoutMode lom )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	uiRect& chldGeomtry = curChld->curpos(lom);
	chldGeomtry.topTo ( rTop );
	chldGeomtry.leftTo ( rLeft );
    }
}

bool i_LayoutMngr::tryToGrowItem( resizeItem& itm, 
				  const int maxhdelt, const int maxvdelt,
				  int hdir, int vdir,
				  const QRect& targetRect, int iternr )
{
    layoutChildren( setGeom );
    uiRect childrenBBox = childrenRect(setGeom);

    uiRect& itmGeometry   = itm.item->curpos( setGeom );
    const uiRect& refGeom = itm.item->curpos( preferred );

    int oldrgt = itmGeometry.right();
    int oldbtm = itmGeometry.bottom();

    if ( hdir<0 && itm.hStr>1 && oldrgt <= targetRect.right() )  hdir = 1;
    if ( vdir<0 && itm.vStr>1 && oldbtm <= targetRect.bottom() ) vdir = 1;

    bool hdone = false;
    bool vdone = false;

    if ( hdir && itm.nhiter>0
        && ( (itm.hStr>1) 
	    || ((itm.hStr==1) && (abs(itm.hDelta+hdir) < abs(maxhdelt)))
	   )
        &&!( (hdir>0) && 
	     ( itmGeometry.right() + hdir > targetRect.right() )
	   )
        &&!( (hdir<0) && 
             ( itmGeometry.right() <= targetRect.right() )
	   )
      ) 
    { 
        hdone = true;
        itm.hDelta += hdir;
        itmGeometry.setWidth ( refGeom.width() + itm.hDelta );
	if ( hdir>0 &&  itmGeometry.right() > targetRect.right() )
	    itmGeometry.leftTo( itmGeometry.left() - 1 );
    }   
    
    if ( vdir && itm.nviter>0 
        && ( (itm.vStr>1) 
            || ((itm.vStr==1) && (abs(itm.vDelta+vdir) < abs(maxvdelt)))
	   )
        &&!( (vdir>0) && 
	       ( itmGeometry.bottom() + vdir > targetRect.bottom() ) )
        &&!( (vdir<0) && 
              ( itmGeometry.bottom() <= targetRect.bottom()) )
      )
    {   
        vdone = true; 
        itm.vDelta += vdir;
        itmGeometry.setHeight( refGeom.height() + itm.vDelta );
    }


    if ( !hdone && !vdone )
	return false;

#ifdef __debug__
    if ( iternr == NR_RES_IT )
    {
	BufferString msg;
	if ( itm.item && itm.item->objLayouted() )
	{   
	    msg="Trying to grow item ";
	    msg+= itm.item->objLayouted() ? 
		    (const char*)itm.item->objLayouted()->name() : "UNKNOWN ";

	    if ( hdone ) msg+=" hdone. ";
	    if ( vdone ) msg+=" vdone. ";
	    if ( itm.nviter ) { msg+=" viter: "; msg += itm.nviter; }
	    if ( itm.nhiter ) { msg+=" hiter: "; msg += itm.nhiter; }

	}
	else msg = "not a uiLayout item..";
	pErrMsg( msg ); 
    }
#endif

    int oldcbbrgt = childrenBBox.right();
    int oldcbbbtm = childrenBBox.bottom();

    layoutChildren( setGeom );
    childrenBBox = childrenRect(setGeom);  

    bool do_layout = false;

    if ( hdone )
    {
	if ( ((hdir >0)&&
             ( 
		( itmGeometry.right() > targetRect.right() )
	      ||(  ( childrenBBox.right() > targetRect.right() ) 
		 &&( childrenBBox.right() > oldcbbrgt )
	        )
	     ) 
	    )
	    || ( (hdir <0) && 
                !(  ( childrenBBox.right() < oldcbbrgt ) 
		  ||(  ( itmGeometry.right() > targetRect.right())
		     &&( itmGeometry.right() < oldrgt)
		    )  
                 )
               )
          )
	{ 
	    itm.nhiter--;
	    itm.hDelta -= hdir;

	    itmGeometry.setWidth( refGeom.width() + itm.hDelta );
	    do_layout = true;
	}
    }

    if ( vdone )
    {
	if ( ((vdir >0)&&
             ( 
		( itmGeometry.bottom() > targetRect.bottom() )
	      ||(  ( childrenBBox.bottom() > targetRect.bottom() ) 
		 &&( childrenBBox.bottom() > oldcbbbtm )
	        )
	     )
	    ) 
	    || ( (vdir <0) && 
                !(  ( childrenBBox.bottom() < oldcbbbtm ) 
		  ||(  ( itmGeometry.bottom() > targetRect.bottom())
		     &&( itmGeometry.bottom() < oldbtm)
		    )
                 )
               )
	  )
	{   
	    itm.nviter--;
	    itm.vDelta -= vdir;

	    itmGeometry.setHeight( refGeom.height() + itm.vDelta );
	    do_layout = true;
	}
    }

    if ( do_layout ) 
    {   // move all items to top-left corner first 
	moveChildrenTo( targetRect.top(), targetRect.left(),setGeom);
	layoutChildren( setGeom );
    } 

    return true;
}


void i_LayoutMngr::resizeTo( const QRect& targetRect, bool isPrefSz )
{
    doLayout( setGeom, targetRect );//init to prefer'd size and initial layout

    const int hgrow = targetRect.width()  - curpos(preferred).hNrPics();
    const int vgrow = targetRect.height() - curpos(preferred).vNrPics();

    const int hdir = ( hgrow >= 0 ) ? 1 : -1;
    const int vdir = ( vgrow >= 0 ) ? 1 : -1;

    ObjectSet<resizeItem> resizeList;
    fillResizeList( resizeList, isPrefSz );

    int iternr = MAX_ITER;

    for( bool go_on = true; go_on && iternr; iternr--)
    {   
	go_on = false;
	for( int idx=0; idx<resizeList.size(); idx++ )
	{
	    resizeItem* cur = resizeList[idx];
	    if ( cur && (cur->nhiter || cur->nviter)) 
	    { 
		if ( tryToGrowItem( *cur, hgrow, vgrow, 
				    hdir, vdir, targetRect, iternr ))
		    go_on = true; 
	    }
	}
    }

    deepErase( resizeList );

    static int printsleft=10;
    if ( !iternr && (printsleft--)>0 )
	{ pErrMsg("Stopped resize. Too many iterations "); }
}

void i_LayoutMngr::setGeometry( const QRect &extRect )
{
    if ( extRect == prevGeometry ) return;

    QRect targetRect = extRect;

    QSize minSz = minimumSize();

    if ( targetRect.width() < minSz.width()  )   
    { targetRect.setWidth( minSz.width()  ); }
    if ( targetRect.height() < minSz.height()  ) 
    { targetRect.setHeight( minSz.height()  ); }

    prevGeometry = targetRect;

    uiRect mPos = curpos( preferred );
    bool isPrefSz = ( extRect.width() == mPos.hNrPics() 
		   && extRect.height() == mPos.vNrPics() 
		   && extRect.left() == mPos.left()    );

    if ( managedBody.uiObjHandle().mainwin()  )
	isPrefSz |= !managedBody.uiObjHandle().mainwin()->poppedUp();

#if 0
    if ( isPrefSz )
	doLayout( setGeom, targetRect );
    else
#endif
	resizeTo( targetRect, isPrefSz );


    layoutChildren( setGeom, true ); // move stuff that's attached to border

    childrenCommitGeometrySet( isPrefSz );
    QLayout::setGeometry( extRect );

}

void i_LayoutMngr::childrenCommitGeometrySet( bool isPrefSz )
{
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	curChld->commitGeometrySet( isPrefSz );
    }
}

void i_LayoutMngr::doLayout( layoutMode lom, const QRect &externalRect )
{
    bool geomSetExt = ( externalRect.width() && externalRect.height() );
    if ( geomSetExt )
    {
	curpos(lom) = uiRect( externalRect.left(), externalRect.top(), 
			    externalRect.right(), externalRect.bottom() );
    }

    int mngrTop  = geomSetExt ? externalRect.top() + borderSpace() 
			      : borderSpace();
    int mngrLeft = geomSetExt ? externalRect.left() + borderSpace()
			      : borderSpace();

    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->initLayout( lom, mngrTop, mngrLeft ); 
    }

    layoutChildren(lom);

    if ( !geomSetExt )
    {
	curpos(lom) = childrenRect(lom);
    }

}

void i_LayoutMngr::layoutChildren( layoutMode lom, bool finalLoop )
{
    int iternr;

    for ( iternr = 0 ; iternr <= MAX_ITER ; iternr++ ) 
    {
        bool child_updated = false;

	for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
	{ 
	    child_updated |= curChld->layout( lom, iternr, finalLoop ); 
	}

	if ( !child_updated )		break;
	if ( finalLoop && iternr > 1 )	break;
    }

    if ( iternr == MAX_ITER ) 
      { pErrMsg("Stopped layout. Too many iterations "); }
}

//! returns rectangle wrapping around all children.
uiRect i_LayoutMngr::childrenRect( layoutMode lom )
{
    uiRect chldRect(-1,-1,-1,-1);

    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    {
	const uiRect* childPos = &curChld->curpos(lom);


	if ( (childPos->top() ) < chldRect.top() || chldRect.top() < 0 ) 
			chldRect.setTop( childPos->top() );
	if ( (childPos->left()) < chldRect.left() || chldRect.left() < 0 ) 
			chldRect.setLeft( childPos->left() );
	if ( childPos->right() > chldRect.right() || chldRect.right() < 0)
				    chldRect.setRight( childPos->right() );
	if ( childPos->bottom()> chldRect.bottom() || chldRect.bottom()< 0)
				    chldRect.setBottom( childPos->bottom());
    }

    if ( int bs = borderSpace() >= 0 )
    {
	int l = mMAX( 0,chldRect.left() - bs );
	int t = mMAX( 0,chldRect.top() - bs );
	int w = chldRect.hNrPics() + 2*bs;
	int h = chldRect.vNrPics() + 2*bs;

	uiRect ret(l,t,0,0);
	ret.setHNrPics(w);
	ret.setVNrPics(h);
	return ret;
    }

    return chldRect;
}


void i_LayoutMngr::invalidate() 
{ 
    prevGeometry = QRect();

    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->invalidate(); 
    }
}

void i_LayoutMngr::updatedAlignment(layoutMode lom)
{ 
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->updatedAlignment(lom);
    }
}

void i_LayoutMngr::initChildLayout(layoutMode lom)
{ 
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
		    i_LayoutItem* curChld = childIter.current(); ++childIter )
    { 
	curChld->initLayout( lom, -1, -1 );
    }
}



//! return true if successful
bool i_LayoutMngr::attach ( constraintType type, QWidget& current, 
			    QWidget* other, int margin) 
{
    if (&current == other)
	{ pErrMsg("Cannot attach an object to itself"); return false; }
    
    i_LayoutItem *cur=0, *oth=0;
    for ( QPtrListIterator<i_LayoutItem> childIter( childrenList );
			i_LayoutItem* loop = childIter.current(); ++childIter )
    {
	if ( loop->qwidget() == &current)	cur = loop;
        if ( loop->qwidget() == other)		oth = loop;
	if ( cur && oth ) break;
    }

    if (cur && ((!oth && !other) || (other && oth && (oth->qwidget()==other)) ))
    {
	cur->attach( type, oth, margin );
	return true;
    }

    const char* curnm =  current.name();
    const char* othnm =  other ? other->name() : "";
    
    BufferString msg( "Cannot attach " );
    msg += curnm;
    msg += " and ";
    msg += othnm;
    pErrMsg( msg );

    return false;
}

