/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.cc,v 1.28 2001-09-28 09:01:56 arend Exp $
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

#define MAX_ITER	10000

#ifdef __debug__
static int max_pos =10000;
#endif

int i_LayoutMngr::mintxtwidgethgt = -1;

#define CHILD_STRETCH_IS_MAX
// how to determine stretch factor for groups? --> max(children)


#define	Qt_misses_some_pixels
// Qt seems to snoop some pixels for toplevel widgets. Correct for this.


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
{ 
    return pos(setGeom).size(); 
}


int i_LayoutItem::stretch( bool hor ) const
{ 
    const uiObjectBody* blo = bodyLayouted();
    return blo ? blo->stretch( hor ) : 0; 
}


void i_LayoutItem::commitGeometrySet()
{
    uiRect mPos = pos( setGeom );

    if( objLayouted() ) objLayouted()->triggerSetGeometry( this, mPos );

//#define DEBUG_LAYOUT 
#ifdef DEBUG_LAYOUT

    BufferString msg;
    if( objLayouted() )
    {   
	msg = "setting geometry on: ";
	msg +=  objLayouted()->name();
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

    QRect geom (mPos.left(), mPos.top(), mPos.hNrPics(), mPos.vNrPics() );

    mQLayoutItem_.setGeometry ( geom ); 

#else

    mQLayoutItem_.setGeometry ( QRect ( mPos.left(), mPos.top(), 
                                        mPos.hNrPics(), mPos.vNrPics() )); 
#endif
}

void i_LayoutItem::initLayout( layoutMode m, int mngrTop, int mngrLeft )
{
    uiRect& mPos = pos( m );
    int pref_h_nr_pics;
    int pref_v_nr_pics;

    if( objLayouted() )
    {
	pref_h_nr_pics	= objLayouted()->prefHNrPics();
	pref_v_nr_pics	= objLayouted()->prefVNrPics();
    }
    else
    {
	QSize sh(mQLayoutItem_.sizeHint());
	pref_h_nr_pics	= sh.width();
	pref_v_nr_pics	= sh.height();
    }

    switch ( m )
    {
	case minimum:
            if( !minimum_pos_inited)
	    {
		mPos.zero();
		mPos.setHNrPics( minimumSize().width() );
		mPos.setVNrPics( minimumSize().height() );
		minimum_pos_inited = true;
	    }
	    break;

	case setGeom:
	    if( !preferred_pos_inited )
	    {
		uiRect& pPos = pos(preferred);
		pPos.setLeft( mngrLeft );
		pPos.setTop( mngrTop );

		pPos.setHNrPics( pref_h_nr_pics  );
		pPos.setVNrPics( pref_v_nr_pics );
		preferred_pos_inited = true;
	    }

	    mPos.setLeft( mngrLeft );
	    mPos.setTop( mngrTop );

	    mPos.setHNrPics( pref_h_nr_pics  );
	    mPos.setVNrPics( pref_v_nr_pics );

	    break;

	case preferred:
	    mPos.setLeft( mngrLeft );
	    mPos.setTop( mngrTop );

	    mPos.setHNrPics( pref_h_nr_pics  );
	    mPos.setVNrPics( pref_v_nr_pics );
	    preferred_pos_inited = true;
	    break;
    } 

    if( mPos.left() < 0 ) 
	{ pErrMsg("left < 0"); }
    if( mPos.top() < 0 ) 
	{ pErrMsg("top < 0"); }



}


#ifdef __debug__

int i_LayoutItem::isPosOk( uiConstraint* c, int i )
{
    if( i <= max_pos ) return i;

    if( c->enabled() ) 
    {
	BufferString msg;

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


#define mCP(val)	isPosOk(constr,(val))
#define mUpdated()	{ isPosOk(constr,MAX_ITER-iteridx); *chupd=true; }

#else

#define mCP(val)	(val)
#define mUpdated()	{ *chupd=true; }

#endif

void i_LayoutItem::layout( layoutMode m, const int iteridx, bool* chupd )
{
//    if ( !constrList ) return;

#define mHorSpacing \
    (constr->margin >= 0 ? constr->margin+1 : mngr_.horSpacing()+1)
#define mVerSpacing \
    (constr->margin >= 0 ? constr->margin+1 : mngr_.verSpacing()+1)


    uiRect& mPos = pos(m);

    constraintIterator it = iterator();

    uiConstraint* constr;
    while ( (constr = it.current()) )
    {
	++it;

	uiRect otherPos = constr->other ? constr->other->pos(m) : uiRect();

	switch ( constr->type )
	{
	    case rightOf:
	    case rightTo:
		if( mPos.leftToAtLeast( mCP(otherPos.right() + mHorSpacing)))  
		    mUpdated(); 
		if ( mPos.topToAtLeast( mCP(otherPos.top()) ) ) 
		     mUpdated();
		break;
	    case leftOf:  
		if( mPos.rightToAtLeast( mCP(otherPos.left() - mHorSpacing)) )  
		    mUpdated(); 
		if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		     mUpdated();
		break;
	    case leftTo:  
		if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		     mUpdated();
		break;
		      
	    case leftAlignedBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated(); 
		if( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		    mUpdated();
		break;

	    case leftAlignedAbove: 
		if( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		    mUpdated();
		break;

	    case rightAlignedBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated();
		if( mPos.rightToAtLeast( mCP(otherPos.right())) )
		    mUpdated();
		break;

	    case rightAlignedAbove: 
		if( mPos.rightToAtLeast( mCP(otherPos.right()) ) )
		    mUpdated();
		break;

	    case alignedBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated();
		if( mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horAlign(m) 
					- horAlign(m)) 
				      )
		  ) 
		    mUpdated();
		break;

	    case alignedAbove: 
		if( mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horAlign(m) 
					- horAlign(m)) 
				      )
		  ) 
		    mUpdated();
		break;

	    case centeredBelow:
	    {
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated(); 

		if( horCentre(m) > 0 && constr->other->horCentre(m) > 0 &&
		    mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horCentre(m) 
					- horCentre(m)) 
				      )
		  ) 
		    mUpdated();
		break;
	    }
	    case centeredAbove: 
            {
		if( horCentre(m) > 0 && constr->other->horCentre(m) > 0 &&
		    mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horCentre(m) 
					- horCentre(m)) 
				      )
		  ) 
		    mUpdated();
		break;
            } 
	    case ensureRightOf:
		if( mPos.leftToAtLeast( mCP(otherPos.right() + mHorSpacing )))  
		    mUpdated(); 
		break;

	    case ensureBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing ))) 
		    mUpdated(); 
		break;
	    case leftBorder:
		{
		    int nwLeft = mngr().pos(m).left() + mHorSpacing;
		    if( mPos.left() != nwLeft )
		    {
			mPos.leftTo( mCP(nwLeft));
			mUpdated();
		    }
		}
		break;
	    case rightBorder:
		{
		    int nwRight = mngr().pos(m).right() - mHorSpacing;
		    if( mPos.rightToAtLeast( mCP(nwRight) ) ) mUpdated();
		}
		break;
	    case topBorder:
		{
		    int nwTop = mngr().pos(m).top() + mVerSpacing;
		    if( mPos.top() != nwTop )
		    {
			mPos.topTo( mCP(nwTop ));
			mUpdated();
		    }
		}
		break;
	    case bottomBorder:
		{
		    int nwBottom = mngr().pos(m).bottom() - mVerSpacing;
		    if( mPos.bottomToAtLeast( mCP(nwBottom )))
			mUpdated();
		}
		break;
	    case heightSameAs:
		if( mPos.height() < ( otherPos.height() ) )
		{
		    mPos.setHeight( otherPos.height() );
		    mUpdated();
		}
		break;
	    case widthSameAs:
		if( mPos.width() < ( otherPos.width() ) )
		{
		    mPos.setWidth( otherPos.width() );
		    mUpdated();
		}
		break;
	    case stretchedBelow:
		{
		    int nwWidth = mngr().pos(m).width();
		    if( mPos.width() < nwWidth )
		    {
			mPos.setWidth( nwWidth );
			mUpdated();
		    }
		    if( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
			mUpdated(); 
		}
		break;
	    case stretchedAbove:
		{
		    int nwWidth = mngr().pos(m).width();
		    if( mPos.width() < nwWidth )
		    {
			mPos.setWidth( nwWidth );
			mUpdated();
		    }
		}
		break;
	    case stretchedLeftTo:
		{
		    int nwHeight = mngr().pos(m).height();
		    if( mPos.height() < nwHeight )
		    {
			mPos.setHeight( nwHeight );
			mUpdated();
		    }
		}
		break;
	    case stretchedRightTo:
		{
		    int nwHeight = mngr().pos(m).height();
		    if( mPos.height() < nwHeight )
		    {
			mPos.setHeight( nwHeight );
			mUpdated();
		    }
		    if( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
			mUpdated(); 
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
			    , prevGeometry()
			    , minimumDone( false )
			    , preferredDone( false )
			    , ismain( parnt ? parnt->isTopLevel() : false )
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
    if ( !minimumDone ) 
    { 
	doLayout( minimum, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->minimumDone=true; 
    }
    uiRect mPos = pos(minimum);
    return QSize( mPos.hNrPics(), mPos.vNrPics() );
}

QSize i_LayoutMngr::sizeHint() const
{
    if ( !preferredDone )
    { 
	doLayout( preferred, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->preferredDone=true; 
    }
    uiRect mPos = pos(preferred);

#ifdef	Qt_misses_some_pixels
/*
    On top-level widgets, Qt appearantly snips of 4 pixels vertically.
*/

    if( ismain )
	return QSize( mPos.hNrPics() + 2 + 10 , mPos.vNrPics() + 6 );
    else
#endif
	return QSize( mPos.hNrPics(), mPos.vNrPics() );
}

QSizePolicy::ExpandData i_LayoutMngr::expanding() const
{
    return QSizePolicy::BothDirections;
}

QLayoutIterator i_LayoutMngr::iterator()
{
    return QLayoutIterator( new i_LayoutIterator( &childrenList ) ) ;
}

//! \internal class used when resizing a window
class resizeItem
{
#define NR_RES_IT	1
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
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	uiObject* cl = curChld->objLayouted();
	if( cl && cl != cb ) cl->clear();
    }
}

int i_LayoutMngr::childStretch( bool hor ) const
{
    QListIterator<i_LayoutItem> childIter( childrenList );

    int sum=0;
    childIter.toFirst();
    while ( i_LayoutItem* curChld = childIter.current() )
    {
        ++childIter;
	uiObjectBody* ccbl = curChld->bodyLayouted();
#ifdef CHILD_STRETCH_IS_MAX
	if( ccbl ) sum = mMAX( sum, ccbl->stretch( hor ) );
#else
	if( ccbl ) sum += ccbl->stretch( hor );
#endif
    }

    return sum;
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

void i_LayoutMngr::fillResizeList( ObjectSet<resizeItem>& resizeList, 
				   int& maxh, int& maxv,
				   int& nrh, int& nrv )
{
    QListIterator<i_LayoutItem> childIter( childrenList );
    childIter.toFirst();

    maxh=0;
    maxv=0;
    nrh=0;
    nrv=0;

    while ( i_LayoutItem* curChld = childIter.current() )
    {
        ++childIter;

#ifdef __debug__
    BufferString nm = curChld->objLayouted() ? 
		    (const char*)curChld->objLayouted()->name() : "unknown" ;
#endif

	int hs = curChld->stretch(true);
	int vs = curChld->stretch(false);

	if ( hs || vs )
        {
	    if( hs )
	    {
		nrh++;
#ifdef CHILD_STRETCH_IS_MAX
		maxh = mMAX( maxh, hs );
#else
		maxh += hs;
#endif
	    }
	    if( vs )
	    {
		nrv++;
#ifdef CHILD_STRETCH_IS_MAX
		maxv = mMAX( maxv, vs );
#else
		maxv += vs;
#endif
	    }

	    resizeList += new resizeItem( curChld, hs, vs );
        }
   } 
}


void i_LayoutMngr::moveChildrenTo(int rTop, int rLeft, layoutMode m )
{
    QListIterator<i_LayoutItem> childIter( childrenList );
    childIter.toFirst();
    while ( i_LayoutItem* curChld = childIter.current() )
    {
	++childIter;
	uiRect& chldGeomtry  = curChld->pos(m);
	chldGeomtry.topTo ( rTop );
	chldGeomtry.leftTo ( rLeft );
    }
}

bool i_LayoutMngr::tryToGrowItem( resizeItem& cur, 
				  const int maxhdelt, const int maxvdelt,
				  int hdir, int vdir,
				  const QRect& targetRect )
{
    BufferString msg;
    msg+= cur.item && cur.item->objLayouted() ? 
		    (const char*)cur.item->objLayouted()->name() : "UNKNOWN";


    layoutChildren( setGeom );
    uiRect childrenBBox = childrenRect(setGeom);  

#if 0
// don't touch groups that have a horalignobj...
    mDynamicCastGet(i_uiGroupLayoutItem*,gli,cur.item);
    if( gli )
	return false;
#endif

    int tgtnrhpx = targetRect.width();
    int tgtnrvpx = targetRect.height();


//    bool done_something = false;

    uiRect& myGeomtry  = cur.item->pos( setGeom );
    const uiRect& minGeom = cur.item->pos( minimum );
    const uiRect& refGeom = cur.item->pos( preferred );

    bool hdone = false;
    bool vdone = false;

    if( cur.hStr>1 && 
		( myGeomtry.right()+hdir+borderSpace() < targetRect.right()) ) 
	hdir = abs(hdir);
    if( cur.vStr>1 && 
		( myGeomtry.bottom()+vdir+borderSpace() < targetRect.bottom()) )
	vdir = abs(vdir);

    //if( doh && cur.nhiter ) 
    if( hdir && cur.nhiter>0
        && ( (cur.hStr>1) || (abs(cur.hDelta+hdir) <= abs(maxhdelt)))
        &&!( (hdir>0) && 
            (  ( myGeomtry.right() + hdir > targetRect.right() )
            || ( childrenBBox.right() > targetRect.right() ) ))
        &&!( (hdir<0) && 
            (  ( myGeomtry.right()+ hdir  < targetRect.right() )
            || ( refGeom.width()+cur.hDelta+hdir <= minGeom.width()) ))
      ) 
    { 
        hdone = true;
        cur.hDelta += hdir;
        myGeomtry.setWidth ( refGeom.width() + cur.hDelta );
    }   
    
    //if(  dov && cur.nviter )
    if( vdir && cur.nviter>0 
        && ( (cur.vStr>1) || abs(cur.vDelta) < abs(maxvdelt))
        &&!( (vdir>0) && 
	    (  ( myGeomtry.bottom()+ vdir > targetRect.bottom())
	    || ( childrenBBox.bottom() > targetRect.bottom() )) )
        &&!( (vdir<0) && 
            (  ( myGeomtry.bottom() + vdir < targetRect.bottom())
            || ( refGeom.height()+cur.vDelta+vdir <= minGeom.height()) ))
      )
    {   
        vdone = true; 
        cur.vDelta += vdir;
        myGeomtry.setHeight( refGeom.height() + cur.vDelta );
    }


    if( !hdone && !vdone )
	return false;
   
    layoutChildren( setGeom );
    childrenBBox = childrenRect(setGeom);  

    bool do_layout = false;

    if( hdone )
    {
	if(   ((hdir >0) && ( childrenBBox.hNrPics() > tgtnrhpx))  
	   || ((hdir <0) && ( childrenBBox.hNrPics() < tgtnrhpx)) 
	   || ((hdir >0) && ( myGeomtry.right() > targetRect.right() )) 
	   || ((hdir >0) && ( childrenBBox.right() > targetRect.right() )) )
	{ 
	    cur.nhiter--;
	    cur.hDelta -= hdir;

	    myGeomtry.setWidth( refGeom.width() + cur.hDelta );
	    do_layout = true;
	}
//	else { done_something = true; }
    }

    if( vdone )
    {
	if(    ((vdir >0) && ( childrenBBox.vNrPics() > tgtnrvpx ) )
	    || ((vdir <0) && ( childrenBBox.vNrPics() < tgtnrvpx ) ) 
	    || ((vdir >0) && ( myGeomtry.bottom() > targetRect.bottom()))
	    || ((vdir >0) && ( childrenBBox.bottom() > targetRect.bottom() )) )
	{   
	    cur.nviter--;
	    cur.vDelta -= vdir;

	    myGeomtry.setHeight( refGeom.height() + cur.vDelta );
	    do_layout = true;
	}
//	else { done_something = true; }
    }

    if( do_layout ) 
    {   // move all items to top-left corner first 
	moveChildrenTo( targetRect.top(), targetRect.left(),setGeom);

	layoutChildren( setGeom );
	childrenBBox = childrenRect(setGeom);  
    } 


    bool doInit=false;

    if( hdone && ((hdir>0)&&( childrenBBox.hNrPics() > tgtnrhpx ))
	      || ((hdir<0)&&(childrenBBox.hNrPics() < tgtnrhpx)) )
	{ doInit=true; }
    if( vdone && ((vdir>0)&&(childrenBBox.vNrPics() > tgtnrvpx )) 
	      || ((vdir<0)&&(childrenBBox.vNrPics() < tgtnrvpx)) )
	{ doInit=true; }

    if( doInit )
    {
	doLayout( setGeom, targetRect );//init to prefer'd size and layout
	childrenBBox = childrenRect(setGeom);  
    }

    if( hdone && ((hdir>0)&&( childrenBBox.hNrPics() > tgtnrhpx ))
	      || ((hdir<0)&&(childrenBBox.hNrPics() < tgtnrhpx)) )
	{ msg+=" grown too wide"; pErrMsg(msg); return false; }
    if( vdone && ((vdir>0)&&(childrenBBox.vNrPics() > tgtnrvpx )) 
	      || ((vdir<0)&&(childrenBBox.vNrPics() < tgtnrvpx)) )
	{ msg+=" grown too tall"; pErrMsg(msg); return false; }


 //   return done_something;
return true;
}


void i_LayoutMngr::resizeTo( const QRect& targetRect )
{
    doLayout( setGeom, targetRect );//init to prefer'd size and initial layout
    uiRect childrenBBox = childrenRect(setGeom);  

#if 0
    const uiRect& refRect = childrenBBox;
#else
    const uiRect& refRect = pos(preferred);
#endif

#if 1
    static int hgrow=0;
    static int vgrow=0;

    if( ismain )
    {
	hgrow = targetRect.width()  - refRect.hNrPics();
	vgrow = targetRect.height() - refRect.vNrPics();
    }
#else

    const int hgrow = targetRect.width()  - refRect.hNrPics();
    const int vgrow = targetRect.height() - refRect.vNrPics();

#endif

#define always_grow
#ifdef always_grow

    const int hdir = ( hgrow >= 0 ) ? 1 : -1;
    const int vdir = ( vgrow >= 0 ) ? 1 : -1;

#else

    if( !hgrow && !vgrow ) return;

    const int hdir = (refRect.hNrPics() == targetRect.width())	? 0 : 
	      ((refRect.hNrPics() < targetRect.width() ) ? 1:-1);
    const int vdir = (refRect.vNrPics() == targetRect.height())	? 0 :
	      (( refRect.vNrPics() < targetRect.height()) ? 1:-1);

    if( !hdir && !vdir )	{ pErrMsg("hoekadanoe?"); return; }

#endif

#if 0
    if( hdir<0 && !ismain )
    {
	static bool print=true;
	if( print ) // print only once
	{
	    print  = false;
	    BufferString msg;
	    msg += UserIDObject::name();
	    msg += "\\";
	    msg += QLayout::name();
	    msg += " is shrinked while we're growing :-((";
	    pErrMsg(msg);
	}
    }
#endif

    ObjectSet<resizeItem> resizeList;
    int maxHstr, maxVstr;
    int nrHstr, nrVstr;
    fillResizeList( resizeList, maxHstr, maxVstr, nrHstr, nrVstr );

    int iter = MAX_ITER;
//    int iter = mMAX(abs(hgrow),abs(vgrow));

    for( bool go_on = true; go_on && iter; iter--)
    {   
	go_on = false;
	for( int idx=0; idx<resizeList.size(); idx++ )
	{
	    resizeItem* cur = resizeList[idx];
	    if( cur && (cur->nhiter || cur->nviter)) 
	    { 
		if( tryToGrowItem( *cur, hgrow, vgrow, 
				    hdir, vdir, targetRect ))
		    go_on = true; 
	    }
	}
    }

    deepErase( resizeList );
    if ( !iter ) pErrMsg("Stopped resize. Too many iterations ");
}

void i_LayoutMngr::setGeometry( const QRect &extRect )
{
    if( extRect == prevGeometry ) return;

    QRect targetRect = extRect;

#ifdef __debug__
    int pref_max = mMAX(pos(preferred).right(),pos(preferred).bottom());
    int ext_max = mMAX(extRect.right(),extRect.bottom() );
    max_pos = mMAX( pref_max, ext_max );
    max_pos += 10; 
#endif

#ifdef	Qt_misses_some_pixels
    if( ismain )
    {
/*
    On toplevel widgets, Qt appearantly uses up some pixels given by sizeHint.
    Unfortunately, this is not always the case. Therefore, we just take some
    extra pixels with the sizehint, and check here for available space.
*/

	int phnp = pos(preferred).hNrPics();
	int pvnp = pos(preferred).vNrPics();

	int hextrapics = extRect.width() - phnp;
	int vextrapics = extRect.height() - pvnp;

	if( hextrapics > 0 && hextrapics <= 10 )
	    targetRect.setWidth( phnp );

	if( vextrapics > 0 && vextrapics <= 10 )
	    targetRect.setHeight( pvnp );

	if( targetRect == prevGeometry ) return;
    }
#endif

    QSize minSz = minimumSize();

    if( targetRect.width() < minSz.width()  )   
    { targetRect.setWidth( minSz.width()  ); }
    if( targetRect.height() < minSz.height()  ) 
    { targetRect.setHeight( minSz.height()  ); }

    prevGeometry = targetRect;

#define do_resize
#ifdef do_resize

    resizeTo( targetRect );

#else
    doLayout( setGeom, targetRect );//init to prefer'd size and initial layout
#endif

    childrenCommitGeometrySet();
    QLayout::setGeometry( extRect );

#ifdef __debug__
    max_pos = 2048;
#endif
}

void i_LayoutMngr::childrenCommitGeometrySet()
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	curChld->commitGeometrySet();
    }
}

void i_LayoutMngr::doLayout( layoutMode m, const QRect &externalRect )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    bool geomSetExt = ( externalRect.width() && externalRect.height() );
    if( geomSetExt )
    {
	pos(m) = uiRect(externalRect.left(), externalRect.top(), 
	    externalRect.right(), externalRect.bottom());
    }

    childIter.toFirst(); 
    while ( (curChld = childIter.current()) ) 
    { 
	++childIter;
	uiObjectBody* cl = curChld->bodyLayouted();
	if( cl && cl->isSingleLine() )
	{ 
	    int chldPref = cl->prefVNrPics();
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
	curChld->initLayout( m, mngrTop, mngrLeft ); 
    }

    layoutChildren(m);

    if( !geomSetExt )
	pos(m) = childrenRect(m);
}

void i_LayoutMngr::layoutChildren( layoutMode m )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    bool child_updated = true;
    int iter = MAX_ITER;
 
    while ( child_updated && iter ) 
    {
        if( iter ) iter--;
        child_updated = false;
        childIter.toFirst();
	while ( (curChld = childIter.current()) )
	{ 
	    ++childIter;
	    curChld->layout(m, iter ,&child_updated); 
	}
	//pos_[ curMode() ] = childrenRect();
    }
    if ( !iter ) 
      { pErrMsg("Stopped layout. Too many iterations "); }
}

#if 0
/*! 

Does a children layout for the case we are layouting a  group that
has a hcentreobj or halignobj specified.

*/
void i_LayoutMngr::doLayoutChildren( layoutMode m, i_uiGroupLayoutItem& it )
{
    uiRect&  mPos = ((i_uiLayoutItem&)it).pos(m);
    QRect geom (mPos.left(), mPos.top(), mPos.hNrPics(), mPos.vNrPics() );
    doLayout(m,geom,true);
}
#endif

uiRect i_LayoutMngr::childrenRect( layoutMode m )
//!< returns rectangle wrapping around all children.
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    curChld = childIter.current();
    uiRect chldRect(-1,-1,-1,-1);
    if( curChld ) 
    {
	const uiRect* childPos = &curChld->pos(m);

	while ( (curChld = childIter.current()) )
	{
	    ++childIter;
	    childPos = &curChld->pos(m);


	    if ( (childPos->top() ) < chldRect.top() || chldRect.top() < 0 ) 
			    chldRect.setTop( childPos->top() );
	    if ( (childPos->left()) < chldRect.left() || chldRect.left() < 0 ) 
			    chldRect.setLeft( childPos->left() );
	    if ( childPos->right() > chldRect.right() || chldRect.right() < 0)
					chldRect.setRight( childPos->right() );
	    if ( childPos->bottom()> chldRect.bottom() || chldRect.bottom()< 0)
					chldRect.setBottom( childPos->bottom());
	}
    }

    chldRect.setRight( chldRect.right() );
    chldRect.setBottom( chldRect.bottom() );

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
