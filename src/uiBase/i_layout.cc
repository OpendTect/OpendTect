/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.cc,v 1.15 2001-08-30 08:22:40 arend Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
#include "errh.h"

#include "i_layout.h"
#include "i_layoutitem.h"
#include "uiobjbody.h"

#include <qlist.h>
#include <qmenubar.h>

#include <stdio.h>
#include <iostream>
#include <limits.h>


#define MAX_ITER 10000


int i_LayoutMngr::mintxtwidgethgt = -1;


//------------------------------------------------------------------------------

i_LayoutItem::i_LayoutItem( i_LayoutMngr& m, QLayoutItem& itm ) 
:   mngr_( m ), mQLayoutItem_( itm ), 
    preferred_pos_inited( false ), minimum_pos_inited( false )
{}


i_LayoutItem::~i_LayoutItem()
{
    delete &mQLayoutItem_;

    constraintIterator it = iterator();
    uiConstraint* c;
    while ( (c = it.current()) )
    {
	++it;
	delete c;
    }
}

void i_LayoutItem::invalidate() 
{ 
    mQLayoutItem_.invalidate();
    preferred_pos_inited = false;
    minimum_pos_inited = false;
}


constraintIterator i_LayoutItem::iterator()
{
    return constraintIterator(constrList);
}


uiSize i_LayoutItem::actualSize( bool include_border ) const
    { return pos_[setGeom].size(); }


int i_LayoutItem::stretch( bool hor )
    { return bodyLayouted()->stretch( hor ); }


void i_LayoutItem::commitGeometrySet()
{
    uiRect mPos = pos_[ setGeom ];

    if( obj2Layout() ) obj2Layout()->triggerSetGeometry( this, mPos );

//#define DEBUG_LAYOUT 
#ifdef DEBUG_LAYOUT

    BufferString msg;
    if( obj2Layout() )
    {   
	msg = "setting geometry on: ";
	msg +=  obj2Layout()->name();
	msg += "; top: ";
        msg += mPos.top();
        msg += " left: ";
        msg += mPos.left();
	msg += " with: ";
        msg += mPos.width();
	msg += " height: ";
        msg += mPos.height();

    }
    else msg = "not a uiLayout item..";
    pErrMsg( msg ); 

    QRect geom ( mPos.left(), mPos.top(), mPos.width(), mPos.height());

    mQLayoutItem_.setGeometry ( geom ); 

#else

    mQLayoutItem_.setGeometry ( QRect ( mPos.left(), mPos.top(), 
                                       mPos.width(), mPos.height() )); 
#endif
}

void i_LayoutItem::initLayout( int mngrTop, int mngrLeft )
{
    uiRect& mPos = pos();
    int preferred_width;
    int preferred_height;

    if( obj2Layout() )
    {
	preferred_width  = obj2Layout()->preferredWidth();
	preferred_height = obj2Layout()->preferredHeight();
    }
    else
    {
	QSize sh(mQLayoutItem_.sizeHint());
	preferred_width  = sh.width();
	preferred_height = sh.height();
    }

#ifdef DEBUG_LAYOUT

    BufferString msg;
    msg = "initLayout on ";
    msg+= obj2Layout() ? const_cast<char*>((const char*)obj2Layout()->name()) : "UNKNOWN";      
    msg+= "  mngrTop: ";
    msg+= mngrTop;
    msg+= "  mngrLeft: ";
    msg+= mngrLeft;

    pErrMsg( msg );

#endif
    switch ( mngr().curMode() )
    {
	case minimum:
            if( !minimum_pos_inited)
	    {
		mPos.zero();
           
                if( stretch(true) )
		    mPos.setWidth( minimumSize().width() );
                else
		    mPos.setWidth( preferred_width );

                if( stretch(false) )
		    mPos.setHeight( minimumSize().height() );
                else
		    mPos.setHeight( preferred_height );

		minimum_pos_inited = true;
	    }
	    break;

	case setGeom:
	    if( !preferred_pos_inited )
	    {
		uiRect& pPos = pos_[preferred];
		pPos.setLeft( mngrLeft );
		pPos.setTop( mngrTop );

		pPos.setWidth ( preferred_width  );
		pPos.setHeight( preferred_height );
		preferred_pos_inited = true;
	    }

	    mPos.setLeft( mngrLeft );
	    mPos.setTop( mngrTop );

	    mPos.setWidth ( preferred_width  );
	    mPos.setHeight( preferred_height );

	    break;

	case preferred:
	    mPos.setLeft( mngrLeft );
	    mPos.setTop( mngrTop );

	    mPos.setWidth ( preferred_width  );
	    mPos.setHeight( preferred_height );
	    preferred_pos_inited = true;
	    break;
    } 
}

void i_LayoutItem::layout()
{
//    if ( !constrList ) return;

#define mHorSpacing (constr->margin >= 0 ? constr->margin :  horSpacing())
#define mVerSpacing (constr->margin >= 0 ? constr->margin :  verSpacing())

    uiRect& mPos = pos();

    constraintIterator it = iterator();

    uiConstraint* constr;
    while ( (constr = it.current()) )
    {
	++it;

	uiRect otherPos = constr->other ? constr->other->pos() : uiRect();

	switch ( constr->type )
	{
	    case rightOf:
	    case rightTo:
		if( mPos.leftToAtLeast( otherPos.right() + mHorSpacing ) )  
		    updated(); 
		if ( mPos.topToAtLeast( otherPos.top() ) ) 
		     updated();
		break;
	    case leftOf:  
		if( mPos.rightToAtLeast( otherPos.left() - mHorSpacing ) )  
		    updated(); 
		if ( mPos.topToAtLeast( otherPos.top() ) ) 
		     updated();
		break;
	    case leftTo:  
		if ( mPos.topToAtLeast( otherPos.top() ) ) 
		     updated();
		break;
		      
	    case leftAlignedBelow:
		if( mPos.topToAtLeast( otherPos.bottom() + mVerSpacing))
		    updated(); 
		if( mPos.leftToAtLeast( otherPos.left() ) ) 
		    updated();
		break;

	    case leftAlignedAbove: 
		if( mPos.leftToAtLeast( otherPos.left() ) ) 
		    updated();
		break;

	    case rightAlignedBelow:
		if( mPos.topToAtLeast( otherPos.bottom() + mVerSpacing))
		    updated();
		if( mPos.rightToAtLeast( otherPos.right() ) )
		    updated();
		break;

	    case rightAlignedAbove: 
		if( mPos.rightToAtLeast( otherPos.right() ) )
		    updated();
		break;

	    case alignedBelow:
		if( mPos.topToAtLeast( otherPos.bottom() + mVerSpacing))
		    updated();
		if( mPos.leftToAtLeast( mPos.left() 
					+ constr->other->horAlign() 
					- horAlign() 
				      )
		  ) 
		    updated();
		break;

	    case alignedAbove: 
		if( mPos.leftToAtLeast( mPos.left() 
					+ constr->other->horAlign() 
					- horAlign() 
				      )
		  ) 
		    updated();
		break;

	    case centeredBelow:
	    {
		if( mPos.topToAtLeast( otherPos.bottom() + mVerSpacing))
		    updated(); 

		if( horCentre() > 0 && constr->other->horCentre() > 0 &&
		    mPos.leftToAtLeast( mPos.left() 
					+ constr->other->horCentre() 
					- horCentre() 
				      )
		  ) 
		    updated();
		break;
	    }
	    case centeredAbove: 
            {
		if( horCentre() > 0 && constr->other->horCentre() > 0 &&
		    mPos.leftToAtLeast( mPos.left() 
					+ constr->other->horCentre() 
					- horCentre() 
				      )
		  ) 
		    updated();
		break;
            } 
	    case ensureRightOf:
		if( mPos.leftToAtLeast( otherPos.right() + mHorSpacing ) )  
		    updated(); 
		break;

	    case ensureBelow:
		if( mPos.topToAtLeast( otherPos.bottom() + mVerSpacing ) ) 
		    updated(); 
		break;
	    case leftBorder:
		{
		    int nwLeft = mngr().pos().left() + mHorSpacing;
		    if( mPos.left() != nwLeft )
		    {
			mPos.leftTo( nwLeft );
			updated();
		    }
		}
		break;
	    case rightBorder:
		{
		    int nwRight = mngr().pos().right() - mHorSpacing;
		    if( mPos.rightToAtLeast( nwRight ) ) updated();
		}
		break;
	    case topBorder:
		{
		    int nwTop = mngr().pos().top() + mVerSpacing;
		    if( mPos.top() != nwTop )
		    {
			mPos.topTo( nwTop );
			updated();
		    }
		}
		break;
	    case bottomBorder:
		{
		    int nwBottom = mngr().pos().bottom() - mVerSpacing;
		    if( mPos.bottomToAtLeast( nwBottom ))
			updated();
		}
		break;
	    case heightSameAs:
		if( mPos.height() < ( otherPos.height() ) )
		{
		    mPos.setHeight( otherPos.height() );
		    updated();
		}
		break;
	    case widthSameAs:
		if( mPos.width() < ( otherPos.width() ) )
		{
		    mPos.setWidth( otherPos.width() );
		    updated();
		}
		break;
	    case stretchedBelow:
		{
		    int nwWidth = mngr().pos().width();
		    if( mPos.width() < nwWidth )
		    {
			mPos.setWidth( nwWidth );
			updated();
		    }
		    if( mPos.topToAtLeast( otherPos.bottom() + mVerSpacing))
			updated(); 
		}
		break;
	    case stretchedAbove:
		{
		    int nwWidth = mngr().pos().width();
		    if( mPos.width() < nwWidth )
		    {
			mPos.setWidth( nwWidth );
			updated();
		    }
		}
		break;
	    case stretchedLeftTo:
		{
		    int nwHeight = mngr().pos().height();
		    if( mPos.height() < nwHeight )
		    {
			mPos.setHeight( nwHeight );
			updated();
		    }
		}
		break;
	    case stretchedRightTo:
		{
		    int nwHeight = mngr().pos().height();
		    if( mPos.height() < nwHeight )
		    {
			mPos.setHeight( nwHeight );
			updated();
		    }
		    if( mPos.leftToAtLeast( otherPos.right() + mHorSpacing ) )  
			updated(); 
		}
		break;
	    case ensureLeftOf:
		break;
	    default:
		pErrMsg("Unknown constraint type");
		break;

	}
    }
}

void i_LayoutItem::attach ( constraintType type, i_LayoutItem *other, 
			    int margn )
{
    if( type != ensureLeftOf)
	constrList.append( new uiConstraint( type, other, margn ) );

    switch ( type )
    {
	case leftOf :
	    other-> constrList.append(new uiConstraint( rightOf, this, margn));
	    break;
	case rightOf:
	    other-> constrList.append(new uiConstraint( leftOf, this, margn ));
	    break;
	case leftTo :
	    other-> constrList.append(new uiConstraint( rightTo, this, margn));
	    break;
	case rightTo:
	    other-> constrList.append(new uiConstraint( leftTo, this, margn ));
	    break;
	case leftAlignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( leftAlignedAbove, this, margn));
	    break;
	case leftAlignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( leftAlignedBelow, this, margn ));
	    break;
	case rightAlignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( rightAlignedAbove, this, margn));
	    break;
	case rightAlignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( rightAlignedBelow, this, margn));
	    break;
	case alignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( alignedAbove, this, margn ) );
	    break;
	case alignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( alignedBelow, this, margn ) );
	    break;
	case centeredBelow:
	    other-> constrList.append( 
			    new uiConstraint( centeredAbove, this, margn ) );
	    break;
	case centeredAbove:
	    other-> constrList.append( 
			    new uiConstraint( centeredBelow, this, margn ) );
	    break;

	case heightSameAs:
	case widthSameAs:
	    other-> constrList.append( new uiConstraint( type, this, margn ) );
	    break;

	case ensureLeftOf:
	    other-> constrList.append( 
			    new uiConstraint( ensureRightOf, this, margn ) );
	    break;
	case stretchedBelow:
	    break;
	case stretchedAbove:
	    other-> constrList.append( 
			    new uiConstraint( ensureBelow, this, margn ) );
	    break;
	case stretchedLeftTo:
	    other-> constrList.append( 
			    new uiConstraint( stretchedRightTo, this, margn ) );
	    break;
	case stretchedRightTo:
	    other-> constrList.append( 
			    new uiConstraint( stretchedLeftTo, this, margn ) );
	    break;

	case leftBorder:
	case rightBorder:
	case topBorder:
	case bottomBorder:
	case ensureRightOf:
	case ensureBelow:
	    break;
	default:
	    pErrMsg("Unknown constraint type");
	    break;
    }
}


//------------------------------------------------------------------

class i_LayoutIterator :public QGLayoutIterator
{
public:
    			i_LayoutIterator( QList<i_LayoutItem> *l ) 
			: idx(0), list(l)  {}

    uint		count() 		const;
    QLayoutItem*	current();
    QLayoutItem*	next();
    QLayoutItem*	takeCurrent();
    i_LayoutItem*	takeCurrent_();

private:
    int idx;
    QList<i_LayoutItem> *list;

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

i_LayoutMngr::i_LayoutMngr( QWidget* parnt, int border, int space,
			    const char *name )
			    : QLayout( parnt, border, space, name)
			    , UserIDObject( name )
			    , curmode( preferred )
			    , prevGeometry()
			    , minimumDone( false )
			    , preferredDone( false )
{}


i_LayoutMngr::~i_LayoutMngr()
{
    i_LayoutIterator it( &childrenList );
    i_LayoutItem *l;
    while ( (l=it.takeCurrent_()) )
	delete l;
}


void i_LayoutMngr::addItem( i_LayoutItem* itm )
{
    if( !itm ) return;
    childrenList.append( itm );
}


/*! \brief Adds a QlayoutItem to the manager's children

    Should normally not been called, since all ui items are added to the
    parent's manager using i_LayoutMngr::addItem( i_LayoutItem* itm )

*/
void i_LayoutMngr::addItem( QLayoutItem *qItem )
{
    if( !qItem ) return;
    childrenList.append( new i_LayoutItem( *this, *qItem) );
}


QSize i_LayoutMngr::minimumSize() const
{
    setMode( minimum ); 
    if ( !minimumDone ) 
    { 
	doLayout( QRect() ); 
	const_cast<i_LayoutMngr*>(this)->minimumDone=true; 
    }
    uiRect mPos = pos();
    return QSize( mPos.width(), mPos.height() );
}

QSize i_LayoutMngr::sizeHint() const
{
    setMode( preferred ); 
    if ( !preferredDone )
    { 
	doLayout( QRect() ); 
	const_cast<i_LayoutMngr*>(this)->preferredDone=true; 
    }
    uiRect mPos = pos();
    return QSize( mPos.width(), mPos.height() );
}

QSizePolicy::ExpandData i_LayoutMngr::expanding() const
{
    return QSizePolicy::NoDirection;
}

QLayoutIterator i_LayoutMngr::iterator()
{
    return QLayoutIterator( new i_LayoutIterator( &childrenList ) ) ;
}

//! \internal class used when resizing a window
class resizeItem
{
public:
			resizeItem( i_LayoutItem* it, int hStr, int vStr ) 
                        : item( it ), nHIt( hStr ? 2 : 0 ), nVIt( vStr ? 2 : 0 )
                        , hDelta( 0 ), vDelta( 0 )  {}

    i_LayoutItem* 	item;
    int 		nHIt;
    int 		nVIt;
    int			hDelta;
    int			vDelta;

};


void i_LayoutMngr::childrenClear( uiObject* cb )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	uiObject* cl = curChld->obj2Layout();
	if( cl && cl != cb ) cl->clear();
    }
}

void i_LayoutMngr::forceChildrenRedraw( uiObjectBody* cb, bool deep )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	uiObjectBody* cl = curChld->bodyLayouted();
	if( cl && cl != cb ) cl->reDraw( deep );
    }

}


void i_LayoutMngr::setGeometry( const QRect &extRect )
{
    if( extRect == prevGeometry ) return;
    prevGeometry = extRect;

    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    QSize minSz = minimumSize();
    QRect targetRect = extRect;

    if( targetRect.width() < minSz.width() )   
    { targetRect.setWidth( minSz.width() ); }
    if( targetRect.height() < minSz.height() ) 
    { targetRect.setHeight( minSz.height() ); }

    QLayout::setGeometry( extRect );
    setMode(setGeom);
    doLayout( targetRect );//init to prefer'd size and initial layout
    uiRect childrenBBox = childrenRect();  

    QList<resizeItem> resizeList;
    QListIterator<resizeItem> resizeListIterator( resizeList );
    resizeList.setAutoDelete( true );

    // make list of resizeable children
    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	if ( curChld->stretch(true) || curChld->stretch(false) )
        {
	    resizeList.append( new resizeItem( curChld, 
			    curChld->stretch(true), curChld->stretch(false)) );
        }
    } 

    int iter = MAX_ITER;

    bool go_on = true;
    while( go_on && iter )
    {   
        if( iter ) iter--;
        go_on = false;

	resizeListIterator.toFirst();
	while( resizeItem* cur = resizeListIterator.current())
	{
            ++resizeListIterator;

	    uiRect& myGeomtry  = cur->item->pos_[ setGeom ];
	    uiRect& minGeomtry = cur->item->pos_[ minimum ];

	    if( cur->nHIt || cur->nVIt )
	    {
		go_on = true;

		if( cur->nHIt ) myGeomtry.setWidth ( minGeomtry.width() + 
						     ++cur->hDelta );
		if( cur->nVIt ) myGeomtry.setHeight( minGeomtry.height() + 
						     ++cur->vDelta );
		setMode( setGeom );	
		layoutChildren( &iter );
		childrenBBox = childrenRect();  
	       
		bool do_layout = false;

		if( cur->nHIt && ( childrenBBox.width() > targetRect.width() ))  
		{ 
		    if( --cur->hDelta < 0 ) cur->hDelta = 0;
		    myGeomtry.setWidth( minGeomtry.width() + cur->hDelta );

		    do_layout = true;
		    cur->nHIt--;
		}

		if( cur->nVIt && (childrenBBox.height() > targetRect.height() ))
		{   
		    if( --cur->vDelta < 0 ) cur->vDelta = 0;
		    myGeomtry.setHeight( minGeomtry.height() + cur->vDelta );

		    do_layout = true;
		    cur->nVIt--; 
		}

		if(do_layout) 
		{   // move all items to top-left corner first 
		    do_layout = false;

		    int rTop = targetRect.top();
		    int rLeft = targetRect.left();

		    childIter.toFirst();
		    while ( (curChld = childIter.current()) )
		    {
			++childIter;
			uiRect& chldGeomtry  = curChld->pos();
			chldGeomtry.topTo ( rTop );
			chldGeomtry.leftTo ( rLeft );
		    }

		    layoutChildren( &iter );
		    childrenBBox = childrenRect();  
		} 
	    }
	}
    }
    resizeList.clear();
    if ( !iter ) pErrMsg("Stopped resize. Too many iterations ");

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	curChld->commitGeometrySet();
    }
}


void i_LayoutMngr::doLayout( const QRect &externalRect )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst(); 
    while ( (curChld = childIter.current()) ) 
    { 
	++childIter;
	uiObjectBody* cl = curChld->bodyLayouted();
	if( cl && cl->isSingleLine() )
	{ 
	    int chldPref = cl->preferredHeight();
	    if( chldPref > mintxtwidgethgt ) 
		mintxtwidgethgt = chldPref;
	}
    }

    int mngrTop  = externalRect.top();
    int mngrLeft = externalRect.left();

    childIter.toFirst(); 
    while ( (curChld = childIter.current()) ) 
    { 
	++childIter; 
	curChld->initLayout( mngrTop, mngrLeft ); 
    }

    layoutChildren();

    pos_[ curMode() ] = childrenRect();
}

void i_LayoutMngr::layoutChildren( int* itr )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    a_child_updated = true;
    int iter = MAX_ITER;
    if( !itr) itr = &iter;
 
    while ( a_child_updated && *itr ) 
    {
        if( *itr ) (*itr)--;

        a_child_updated = false;
        childIter.toFirst();
	while ( (curChld = childIter.current()) )
	{ 
	    ++childIter;
	    curChld->layout(); 
	}
	//pos_[ curMode() ] = childrenRect();
    }
    if ( !iter ) pErrMsg("Stopped layout. Too many iterations ");
}

uiRect i_LayoutMngr::childrenRect()
//!< returns rectangle wrapping around all children.
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    curChld = childIter.current();
    uiRect chldRect;
    if( curChld ) 
    {
	const uiRect* childPos = &curChld->pos();

	chldRect.setTopLeft( childPos->topLeft() );
	chldRect.setBottomRight( childPos->bottomRight() );

	while ( (curChld = childIter.current()) )
	{
	    ++childIter;
	    childPos = &curChld->pos();


	    if ( childPos->top() < chldRect.top() ) 
					chldRect.setTop( childPos->top() );
	    if ( childPos->left() < chldRect.left() ) 
					chldRect.setLeft( childPos->left() );
	    if ( childPos->right() > chldRect.right() ) 
					chldRect.setRight( childPos->right() );
	    if ( childPos->bottom() > chldRect.bottom() ) 
					chldRect.setBottom( childPos->bottom());
	}
    }
    return chldRect;
}


void i_LayoutMngr::invalidate() 
{ 

    prevGeometry = QRect();
    minimumDone = false;
    preferredDone = false;

    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );
    childIter.toFirst(); 

    while ( (curChld = childIter.current()) ) 
    { 
	++childIter; 
	curChld->invalidate(); 
    }
}


bool i_LayoutMngr::attach ( constraintType type, QWidget& current, 
			    QWidget* other, int margin) 
//!< \return true if successful
{
    QListIterator<i_LayoutItem> childIter( childrenList );
   
    childIter.toFirst();
    i_LayoutItem *loop, *cur=0, *oth=0;

    if (&current == other)
	{ pErrMsg("Cannot attach an object to itself"); return false; }
    
    while ( ( loop = childIter.current() ) && !(cur && oth) ) 
    {
        ++childIter;
	if( loop->qwidget() == &current) cur = loop;
        if( loop->qwidget() == other)   oth = loop;
    }

    if (cur && ((!oth && !other) || (other && oth && (oth->qwidget()==other)) ))
    {
	cur->attach( type, oth, margin );
	return true;
    }

    const char* curnm =  current.name();
    const char* othnm =  other ? other->name() : "";
    
    BufferString msg( "i_LayoutMngr :: Cannot attach " );
    msg += curnm;
    msg += " and ";
    msg += othnm;
    pErrMsg( msg );

    return false;
}

//-----------------------------------------------------------------------------
//  Documentation 
//-----------------------------------------------------------------------------

