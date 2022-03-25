/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          18/08/1999
________________________________________________________________________

-*/

#include "i_layout.h"
#include "i_layoutitem.h"

#include "uilayout.h"
#include "uimainwin.h"
#include "uiobjbody.h"

#include "envvars.h"
#include "od_ostream.h"
#include "settings.h"
#include "timer.h"

#ifdef __debug__
#define MAX_ITER	8000	//TODO: Increased from 2000 because of PEs
				//in PS Viewer. Check how to lower it again.
static bool lyoutdbg = false;
#else
#define MAX_ITER	20000
#endif

mUseQtnamespace

i_LayoutMngr::i_LayoutMngr( QWidget* parnt, const char* nm,
			    uiObjectBody& mngbdy )
    : QLayout(parnt)
    , NamedCallBacker(nm)
    , managedbody_(mngbdy)
    , poptimer_(*new Timer)
{
#ifdef __debug__
    mDefineStaticLocalObject( bool, lyoutdbg_loc,
			      = GetEnvVarYN("DTECT_DEBUG_LAYOUT") );
    lyoutdbg = lyoutdbg_loc;
#endif

    vspacing_ = 2;
    mSettUse(get,"dTect","GUI VSpacing",vspacing_);
    if ( vspacing_<1 || vspacing_>10 )
	vspacing_ = 2;

    mAttachCB( poptimer_.tick, i_LayoutMngr::popTimTick );
}


i_LayoutMngr::~i_LayoutMngr()
{
    detachAllNotifiers();
    for ( auto* children : childrenlist_ )
	children->mngr_ = nullptr;

    delete &poptimer_;
}


bool i_LayoutMngr::isMainWinFinalized() const
{
    return managedbody_.uiObjHandle().mainwin() ?
	managedbody_.uiObjHandle().mainwin()->finalized() : true;
}


void i_LayoutMngr::addItem( i_LayoutItem* itm )
{
    if ( !itm )
	return;

    mAttachCB( itm->objectToBeDeleted(), i_LayoutMngr::itemDel );
    childrenlist_ += itm;
}


/*! \brief Adds a QlayoutItem to the manager's children

    Should normally not been called, since all ui items are added to the
    parent's manager using i_LayoutMngr::addItem( i_LayoutItem* itm )

*/
void i_LayoutMngr::addItem( QLayoutItem* qItem )
{
    if ( qItem )
	addItem( new i_LayoutItem(*this,*qItem) );
}


void i_LayoutMngr::itemDel( CallBacker* cb )
{
    if ( !cb )
	return;

    i_LayoutItem* itm = static_cast<i_LayoutItem*>( cb );
    if ( !itm )
	{ pErrMsg("huh?"); return; }

    childrenlist_ -= itm;
}


QSize i_LayoutMngr::minimumSize() const
{
    if ( !isMainWinFinalized() )
	return QSize(0,0);

    if ( !minimumdone_ )
    {
	doLayout( minimum, QRect() );
	const_cast<i_LayoutMngr*>(this)->minimumdone_ = true;
    }

    uiRect mPos;
    if ( ismain_ )
    {
	if ( managedbody_.shrinkAllowed() )
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

#ifdef __debug__
	if ( lyoutdbg )
	{
	    BufferString msg;
	    msg="Returning Minimum Size for ";
	    msg += NamedCallBacker::name();
	    msg += ". (h,v)=(";
	    msg += hsz;
	    msg +=" , ";
	    msg += vsz;
	    msg += ").";

	    od_cout() << msg << od_endl;
	}
#endif
	return QSize( hsz, vsz );
    }

    mPos = curpos( minimum );
    return QSize( mPos.hNrPics(), mPos.vNrPics() );
}


QSize i_LayoutMngr::sizeHint() const
{
    if ( !isMainWinFinalized() )
	return QSize(0, 0);

    if ( !preferreddone_ )
    {
	doLayout( preferred, QRect() );
	const_cast<i_LayoutMngr*>(this)->preferreddone_ = true;
    }
    uiRect mPos = curpos(preferred);

#ifdef __debug__

    if ( mPos.hNrPics() > 4096 || mPos.vNrPics() > 4096 )
    {
	BufferString msg;
	msg="Very large preferred size for ";
	msg += NamedCallBacker::name();
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


const uiRect& i_LayoutMngr::curpos( LayoutMode lom ) const
{
    i_LayoutItem* managedItem =
	    const_cast<i_LayoutItem*>( managedbody_.layoutItem() );

    return managedItem ? managedItem->curpos(lom) : layoutpos_[lom];
}


uiRect& i_LayoutMngr::curpos( LayoutMode lom )
{
    i_LayoutItem* managedItem =
	    const_cast<i_LayoutItem*>(managedbody_.layoutItem());

    return managedItem ? managedItem->curpos(lom) : layoutpos_[lom];
}


uiRect i_LayoutMngr::winpos( LayoutMode lom ) const
{
    i_LayoutItem* managedItem =
	    const_cast<i_LayoutItem*>(managedbody_.layoutItem());

    if ( ismain_ && managedItem )
	return managedItem->curpos(lom);

    if ( !managedItem )
    {
	int hborder = layoutpos_[lom].left();
	int vborder = layoutpos_[lom].top();
	return uiRect( 0, 0, layoutpos_[lom].right()+2*hborder,
			     layoutpos_[lom].bottom()+2*vborder );
    }

    return managedItem->mngr().winpos( lom );
}


//! \internal class used when resizing a window
class ResizeItem
{
#define NR_RES_IT	3
public:
			ResizeItem( i_LayoutItem* it, int hStre, int vStre )
			    : item( it ), hStr( hStre ), vStr( vStre )
			    , hDelta( 0 ), vDelta( 0 )
			    , nhiter( hStre ? NR_RES_IT : 0 )
			    , nviter( vStre ? NR_RES_IT : 0 ) {}

    i_LayoutItem*	item;
    const int		hStr;
    const int		vStr;
    int			nhiter;
    int			nviter;
    int			hDelta;
    int			vDelta;

};


void i_LayoutMngr::childrenClear( uiObject* cb )
{
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	uiObject* cl = childrenlist_[idx]->objLayouted();
	if ( cl && cl != cb ) cl->clear();
    }
}


bool i_LayoutMngr::isChild( uiObject* obj )
{
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	uiObject* cl = childrenlist_[idx]->objLayouted();
	if ( cl && cl == obj ) return true;
    }
    return false;
}


int i_LayoutMngr::childStretch( bool hor ) const
{
    int max=0;

    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	const uiObjectBody* ccbl = childrenlist_[idx]->bodyLayouted();
	if ( ccbl )
	{
	    int cs = ccbl->stretch( hor );
	    if ( cs < 0 || cs > 2 ) { cs = 0; }
	    max = mMAX( max, cs );
	}
    }

    return max;
}


int i_LayoutMngr::horSpacing() const
{
    return hspacing_ >= 0 ? hspacing_ :  managedbody_.fontWidth();
}


void i_LayoutMngr::forceChildrenRedraw( uiObjectBody* cb, bool deep )
{
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	uiObjectBody* cl = childrenlist_[idx]->bodyLayouted();
	if ( cl && cl != cb ) cl->reDraw( deep );
    }
}


void i_LayoutMngr::fillResizeList( ObjectSet<ResizeItem>& resizeList,
				   bool isPrefSz )
{
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	int hs = childrenlist_[idx]->stretch(true);
	int vs = childrenlist_[idx]->stretch(false);
	if ( hs || vs )
	{
	    bool add=false;

	    if ( (hs>1) || (hs==1 && !isPrefSz) )
		add = true;
	    else
		hs=0;

	    if ( (vs>1) || (vs==1 && !isPrefSz) )
		add = true;
	    else
		vs=0;

	    if ( add )
		resizeList += new ResizeItem( childrenlist_[idx], hs, vs );
	}
    }
}


void i_LayoutMngr::moveChildrenTo(int rTop, int rLeft, LayoutMode lom )
{
    for ( int idx=0; idx < childrenlist_.size(); idx++ )
    {
	uiRect& chldGeomtry = childrenlist_[idx]->curpos(lom);
	chldGeomtry.topTo ( rTop );
	chldGeomtry.leftTo ( rLeft );
    }
}


bool i_LayoutMngr::tryToGrowItem( ResizeItem& itm,
				  int maxhdelt, int maxvdelt,
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
        itmGeometry.setHNrPics ( refGeom.hNrPics() + itm.hDelta );
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
        itmGeometry.setVNrPics( refGeom.vNrPics() + itm.vDelta );
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
	bool revert = false;

	if ( hdir > 0 )
	{
	    revert |= itmGeometry.right() > targetRect.right();

	    bool tmp = childrenBBox.right() > targetRect.right();
	    tmp &= childrenBBox.right() > oldcbbrgt;

	    revert |= tmp;
	}

	if( hdir < 0 )
	{
	    bool tmp =  childrenBBox.right() <= oldcbbrgt ;
/*
	    bool tmp2 = itmGeometry.right() > targetRect.right();
		tmp2 &= itmGeometry.right() < oldrgt;
*/
	    revert = !tmp;
	}

	if( revert )
	{
	    itm.nhiter--;
	    itm.hDelta -= hdir;

	    itmGeometry.setHNrPics( refGeom.hNrPics() + itm.hDelta );
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

	    itmGeometry.setVNrPics( refGeom.vNrPics() + itm.vDelta );
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


void i_LayoutMngr::resizeTo( const QRect& targetRect )
{
    doLayout( setGeom, targetRect );//init to prefer'd size and initial layout

    const int hgrow = targetRect.width()  - curpos(preferred).hNrPics();
    const int vgrow = targetRect.height() - curpos(preferred).vNrPics();

    const int hdir = ( hgrow >= 0 ) ? 1 : -1;
    const int vdir = ( vgrow >= 0 ) ? 1 : -1;

    bool isprefsz = !hgrow && !vgrow;
    isprefsz |= !poppedup_;


#ifdef __debug__
    if ( lyoutdbg )
    {
	od_cout() << "(Re)sizing:" << NamedCallBacker::name();
	if ( isprefsz ) od_cout() << " yes"; else
	    { od_cout() << " no " << hgrow << " ," << vgrow; }
	od_cout() << od_endl;
    }
#endif

    ObjectSet<ResizeItem> resizeList;
    fillResizeList( resizeList, isprefsz );

    int iternr = MAX_ITER;

    for( bool go_on = true; go_on && iternr; iternr--)
    {
	go_on = false;
	for( int idx=0; idx<resizeList.size(); idx++ )
	{
	    ResizeItem* cur = resizeList[idx];
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
    if ( !isMainWinFinalized() )
	return;

#ifdef __debug__
    if ( lyoutdbg )
    {
	od_cout() << "setGeometry called on: ";
	od_cout() << NamedCallBacker::name() << od_endl;

	od_cout() << "l: " << extRect.left() << " t: " << extRect.top();
	od_cout() << " hor: " << extRect.width();
	od_cout() << " ver: " << extRect.height() << od_endl;

    }
#endif

    resizeTo( extRect );
    layoutChildren( setGeom, true ); // move stuff that's attached to border

    bool store2prefpos = false;
    if( !prefposstored_ || !poppedup_ )
    {
	uiRect mPos = curpos( preferred );

	int hdif = abs( extRect.width() - mPos.hNrPics() );
	int vdif = abs( extRect.height() - mPos.vNrPics() );

	store2prefpos = !prefposstored_ || (hdif<10 && vdif<10);

#ifdef __debug__
	if ( lyoutdbg && !store2prefpos )
	{
	    od_cout() << "setGeometry called with wrong size on: ";
	    od_cout() << NamedCallBacker::name() << od_endl;
	    od_cout() << "Width should be " << mPos.hNrPics();
	    od_cout() << ", is " << extRect.width();
	    od_cout() << ". Height should be " << mPos.vNrPics();
	    od_cout() << ", is " << extRect.height();
	    od_cout() << od_endl;
	}
#endif
    }

    childrenCommitGeometrySet( store2prefpos );
    if ( store2prefpos )
    {
	prefgeometry_ = extRect;
	prefposstored_ = true;
    }

    QLayout::setGeometry( extRect );
}


void i_LayoutMngr::childrenCommitGeometrySet( bool store2prefpos )
{
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
	 childrenlist_[idx]->commitGeometrySet( store2prefpos );
}


void i_LayoutMngr::doLayout( LayoutMode lom, const QRect& extrect )
{
    bool geomSetExt = extrect.width()>0 && extrect.height()>0;
    if ( geomSetExt )
	curpos(lom) = uiRect( extrect.left(), extrect.top(),
			      extrect.right(), extrect.bottom() );

    int mngrTop  = geomSetExt ? extrect.top() + borderSpace() : borderSpace();
    int mngrLeft = geomSetExt ? extrect.left() + borderSpace() : borderSpace();

    for ( int idx=0; idx<childrenlist_.size(); idx++ )
	 childrenlist_[idx]->initLayout( lom, mngrTop, mngrLeft );

    layoutChildren(lom);
    if ( !geomSetExt )
	curpos(lom) = childrenRect(lom);
}


void i_LayoutMngr::layoutChildren( LayoutMode lom, bool finalLoop )
{
    startPoptimer();

    int iternr;
    for ( iternr=0 ; iternr<MAX_ITER; iternr++ )
    {
        bool child_updated = false;
	for ( int idx=0; idx<childrenlist_.size(); idx++ )
	{
	    child_updated |=
		childrenlist_[idx]->layout( lom, iternr, finalLoop );
	}

	if ( !child_updated )		break;
	if ( finalLoop && iternr > 1 )	break;
    }

    if ( iternr == MAX_ITER )
      { pErrMsg("Stopped layout. Too many iterations "); }
}


//! returns rectangle wrapping around all children.
uiRect i_LayoutMngr::childrenRect( LayoutMode lom )
{
    uiRect chldRect(-1,-1,-1,-1);

    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	const uiRect* childPos = &childrenlist_[idx]->curpos(lom);

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
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
	 childrenlist_[idx]->invalidate();
}


void i_LayoutMngr::updatedAlignment( LayoutMode lom )
{
    for ( int idx=0; idx < childrenlist_.size(); idx++ )
	 childrenlist_[idx]->updatedAlignment(lom);
}


void i_LayoutMngr::initChildLayout( LayoutMode lom )
{
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
	 childrenlist_[idx]->initLayout( lom, -1, -1 );
}


QLayoutItem* i_LayoutMngr::itemAt( int idx ) const
{
    if ( childrenlist_.validIdx(idx) && childrenlist_[idx] )
	return const_cast<QLayoutItem*>(&childrenlist_[idx]->qlayoutItm());
    return 0;
}


QLayoutItem* i_LayoutMngr::takeAt( int idx )
{
    i_LayoutItem* itm = childrenlist_[idx];
    childrenlist_ -= itm;

    QLayoutItem* ret = itm->takeQlayoutItm(); delete itm;
    return ret;
}


int i_LayoutMngr::count () const
    { return childrenlist_.size(); }


bool i_LayoutMngr::attach( constraintType type, QWidget& current,
			   QWidget* other, int mrgin,
			   bool reciprocal )
{
    if ( &current == other )
	{ pErrMsg("Attempt to attach an object to itself"); return false; }

    i_LayoutItem* curli = 0; i_LayoutItem* othli = 0;

    const bool needother = other;
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	i_LayoutItem* child = childrenlist_[idx];
	if ( child->qwidget() == &current )
	    curli = child;
	else if ( needother && child->qwidget() == other )
	    othli = child;

	if ( curli && (!needother || othli) )
	{
	    curli->attach( type, othli, mrgin, reciprocal );
	    return true;
	}
    }

    BufferString msg( NamedCallBacker::name() ); msg += ": Cannot attach '";
    msg += current.objectName(); msg += "'";
    if ( needother )
    {
	msg += " and '";
	msg += other->objectName(); msg += "'";
    }

    msg += " - constraint: "; msg += (int)type;
    msg += "\nChildren are:";
    for ( int idx=0; idx<childrenlist_.size(); idx++ )
    {
	i_LayoutItem* child = childrenlist_[idx];
	msg += "\n"; msg += child->name();
    }

    pErrMsg( msg );
    return false;
}


void i_LayoutMngr::popTimTick( CallBacker* )
{
    timerrunning_ = false;
    if ( poppedup_ ) { pErrMsg( "huh?" ); }
        poppedup_ = true;
}


void i_LayoutMngr::startPoptimer()
{
    if ( timerrunning_ || poppedup_ ) return;

    if ( managedbody_.uiObjHandle().mainwin()
	 && !managedbody_.uiObjHandle().mainwin()->touch() )
	return;

    if ( poptimer_.isActive() )
	poptimer_.stop();

    poptimer_.start( 100, true );
    timerrunning_ = true;
}
