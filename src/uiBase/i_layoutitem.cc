/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/02/2003
________________________________________________________________________

-*/

#include "i_layoutitem.h"
#include "i_layout.h"

#include "uilayout.h"
#include "uimainwin.h"
#include "uiobjbody.h"

#include "envvars.h"
#include "od_ostream.h"

#ifdef __debug__
# define MAX_ITER	2000
static bool lyoutdbg = false;
#else
# define MAX_ITER	10000
#endif

mUseQtnamespace

//------------------------------------------------------------------------------

i_LayoutItem::i_LayoutItem( i_LayoutMngr& mgr, QLayoutItem& itm )
    : NamedCallBacker()
    , qlayoutitm_(&itm)
    , mngr_(&mgr)
{
#ifdef __debug__
    mDefineStaticLocalObject( bool, lyoutdbg_loc,
			      = GetEnvVarYN("DTECT_DEBUG_LAYOUT") );
    lyoutdbg = lyoutdbg_loc;
#endif

    mAttachCB( mgr.objectToBeDeleted(), i_LayoutItem::managerDeletedCB );
}


i_LayoutItem::~i_LayoutItem()
{
    sendDelNotif();
    detachAllNotifiers();
    delete qlayoutitm_;
}


void i_LayoutItem::managerDeletedCB( CallBacker* )
{
    mngr_ = nullptr;
}


int i_LayoutItem::center( LayoutMode lm, bool hor ) const
{
    if ( hor )
	return (curpos(lm).left() + curpos(lm).right()) / 2;

    return (curpos(lm).top() + curpos(lm).bottom()) / 2;
}


uiSize i_LayoutItem::minimumSize() const
{
    mQtclass(QSize) s = qwidget()->minimumSize();
    return uiSize( s.width(), s.height() );
}


uiSize i_LayoutItem::prefSize() const
{
    if ( prefszdone_ )
    {
	pErrMsg("PrefSize already done.");
    }
    else
    {
	i_LayoutItem* self = const_cast<i_LayoutItem*>(this);
	self->prefszdone_ = true;
	mQtclass(QSize) ps( qlayoutItm().sizeHint() );
	int width = ps.width();
	if ( width==0 ) width = 1;
	int height = ps.height();
	if ( height==0 ) height = 1;
	self->prefsz_ = uiSize(width,height);
    }

    return prefsz_;
}


void i_LayoutItem::invalidate()
{
    qlayoutitm_->invalidate();
    preferred_pos_inited_ = false;
}


uiSize i_LayoutItem::actualSize( bool include_border ) const
{
    return curpos(setGeom).getPixelSize();
}


bool i_LayoutItem::inited() const
{
    return minimum_pos_inited_ || preferred_pos_inited_;
}


int i_LayoutItem::stretch( bool hor ) const
{
    if ( (hor && hsameas_) || (!hor && vsameas_) )
	return 0;

    const uiObjectBody* blo = bodyLayouted();
    return blo ? blo->stretch( hor ) : 0;
}


void i_LayoutItem::commitGeometrySet( bool store2prefpos )
{
#ifdef __debug__
    const BufferString objnm = objLayouted()->name();
#endif
    uiRect itmgeom = curpos( setGeom );

    if ( store2prefpos )
	curpos( preferred ) = itmgeom;

    if ( objLayouted() )
	objLayouted()->triggerSetGeometry( this, itmgeom );

#ifdef __debug__
    if ( lyoutdbg )
    {
	od_cout() << "Setting layout on: ";
	if ( objLayouted() )
	    od_cout() << objnm << od_endl;
	else
	    od_cout() << "Unknown" << od_endl;

	od_cout() << "l: " << itmgeom.left() << " t: " << itmgeom.top();
	od_cout() << " hor: " << itmgeom.hNrPics() << " ver: "
			 << itmgeom.vNrPics() << od_endl;
    }
#endif

    const QRect qgeom( itmgeom.left(), itmgeom.top(),
		       itmgeom.hNrPics(), itmgeom.vNrPics());
    qlayoutitm_->setGeometry( qgeom );
}


void i_LayoutItem::initLayout( LayoutMode lom, int mngrtop, int mngrleft )
{
    uiRect& itmgeom = curpos( lom );
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
	    pref_h_nr_pics	= prefSize().hNrPics();
	    pref_v_nr_pics	= prefSize().vNrPics();
	}
    }

#ifdef __debug__
    if ( lyoutdbg )
    {
	BufferString blnm = bodyLayouted() ?  bodyLayouted()->name().buf()
					   : "";

	od_cout() << "Init layout on: " << blnm;
	od_cout() << ": prf hsz: " << pref_h_nr_pics;
	od_cout() <<",  prf vsz: " << pref_v_nr_pics;
	od_cout() <<", mngr top: " << mngrtop;
	od_cout() <<", mngr left: " << mngrleft;
	od_cout() <<",  layout mode: " << (int) lom << od_endl;
    }
#endif

    switch ( lom )
    {
	case minimum:
	    if ( !minimum_pos_inited_)
	    {
		itmgeom.zero();
		uiSize ms = minimumSize();
		itmgeom.setHNrPics( ms.hNrPics() );
		itmgeom.setVNrPics( ms.vNrPics() );
		minimum_pos_inited_ = true;
	    }
	    break;

	case setGeom:
	    {
		uiRect& pPos = curpos(preferred);
		if ( !preferred_pos_inited_ )
		{
		    pPos.setLeft( 0 );
		    pPos.setTop( 0 );

		    pPos.setHNrPics( pref_h_nr_pics  );
		    pPos.setVNrPics( pref_v_nr_pics );
		    preferred_pos_inited_ = true;
		}
		uiRect& itmgeom2 = curpos( lom );
		itmgeom2 = curpos( preferred );

		itmgeom2.leftTo( mMAX(pPos.left(),mngrleft) );
		itmgeom2.topTo( mMAX(pPos.top(),mngrtop) );

		initChildLayout(lom);
	    }
	    break;

	case preferred:
	    {
		itmgeom.setLeft( mngrleft );
		itmgeom.setTop( mngrtop );

		itmgeom.setHNrPics( pref_h_nr_pics  );
		itmgeom.setVNrPics( pref_v_nr_pics );
		preferred_pos_inited_ = true;
	    }
	    break;
	case all:
	    break;
    }

    if ( itmgeom.left() < 0 )
	{ pErrMsg("left < 0"); }
    if ( itmgeom.top() < 0 )
	{ pErrMsg("top < 0"); }
}


#ifdef __debug__

int i_LayoutItem::isPosOk( uiConstraint* constraint, int iter, bool chknriters )
{
    if ( (chknriters && iter<MAX_ITER) || iter <= 2000 )
	return iter;

    if ( constraint->enabled() )
    {
	BufferString msg;
	if ( chknriters )
	    msg = "\n  Too many iterations with: \"";
	else
	    msg = "\n  Layout loop on: \"";
	msg += objLayouted() ? (const char*)objLayouted()->name() : "UNKNOWN";
	msg += "\"";

	switch ( constraint->type_ )
	{
	    case leftOf:		msg += " leftOf "; break;
	    case rightOf:		msg += " rightOf "; break;
	    case leftTo:		msg += " leftTo "; break;
	    case rightTo:		msg += " rightTo "; break;
	    case leftAlignedBelow:	msg += " leftAlignedBelow "; break;
	    case leftAlignedAbove:	msg += " leftAlignedAbove "; break;
	    case rightAlignedBelow:	msg += " rightAlignedBelow "; break;
	    case rightAlignedAbove:	msg += " rightAlignedAbove "; break;
	    case alignedBelow:		msg += " alignedBelow "; break;
	    case alignedAbove:		msg += " alignedAbove "; break;
	    case centeredBelow:		msg += " centeredBelow "; break;
	    case centeredAbove:		msg += " centeredAbove "; break;
	    case ensureLeftOf:		msg += " ensureLeftOf "; break;
	    case ensureRightOf:		msg += " ensureRightOf "; break;
	    case ensureBelow:		msg += " ensureBelow "; break;
	    case leftBorder:		msg += " leftBorder "; break;
	    case rightBorder:		msg += " rightBorder "; break;
	    case topBorder:		msg += " topBorder "; break;
	    case bottomBorder:		msg += " bottomBorder "; break;
	    case heightSameAs:		msg += " heightSameAs "; break;
	    case widthSameAs:		msg += " widthSameAs "; break;
	    case stretchedBelow:	msg += " stretchedBelow "; break;
	    case stretchedAbove:	msg += " stretchedAbove "; break;
	    case stretchedLeftTo:	msg += " stretchedLeftTo "; break;
	    case stretchedRightTo:	msg += " stretchedRightTo "; break;
	    case atSamePosition:	msg += " atSamePosition "; break;
	    default:			msg += " .. "; break;
	}

	msg += "\"";
	msg += constraint->other_ && constraint->other_->objLayouted()
	    ? (const char*)constraint->other_->objLayouted()->name()
	    : "UNKNOWN";
	msg += "\"";
	pErrMsg( msg );

	constraint->disable( true );
    }

    return iter;
}


#define mCP(val)	isPosOk(constr,(val),false)
#define mUpdated()	{ isPosOk(constr,iternr,true); updated=true; }

#else

#define mCP(val)	(val)
#define mUpdated()	{ updated=true; }

#endif


#define mHorSpacing (constr->margin_>=0 ? constr->margin_ : mngr_->horSpacing())
#define mVerSpacing (constr->margin_>=0 ? constr->margin_ : mngr_->verSpacing())

#define mFullStretch() (constr->margin_ < -1)
#define mInsideBorder  (constr->margin_ > mngr_->borderSpace() \
			 ? constr->margin_ - mngr_->borderSpace() : 0)

bool i_LayoutItem::layout( LayoutMode lom, const int iternr, bool finalloop )
{
    bool updated = false;
    uiRect& itmgeom = curpos(lom);

    for ( int idx=0; idx<constraintlist_.size(); idx++ )
    {
	uiConstraint* constr = &constraintlist_[idx];
	const uiRect& otherPos
		= constr->other_ ? constr->other_->curpos(lom) : curpos(lom);

	switch ( constr->type_ )
	{
	case rightOf:
	case rightTo:
	{
	    if ( itmgeom.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated();

	    if ( itmgeom.topToAtLeast( mCP(otherPos.top()) ) )
		 mUpdated();

	    break;
	}
	case leftOf:
	{
	    if ( itmgeom.rightToAtLeast(mCP(otherPos.left() - mHorSpacing)))
		mUpdated();

	    if ( itmgeom.topToAtLeast( mCP(otherPos.top())) )
		 mUpdated();

	    break;
	}
	case leftTo:
	{
	    if ( itmgeom.topToAtLeast( mCP(otherPos.top())) )
		 mUpdated();

	    break;
	}
	case leftAlignedBelow:
	{
	    if ( itmgeom.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    if ( itmgeom.leftToAtLeast( mCP(otherPos.left())) )
		mUpdated();

	    break;
	}
	case leftAlignedAbove:
	{
	    if ( itmgeom.leftToAtLeast( mCP(otherPos.left())) )
		mUpdated();

	    break;
	}
	case rightAlignedBelow:
	{
	    if ( itmgeom.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    if ( itmgeom.rightToAtLeast( mCP(otherPos.right())) )
		mUpdated();

	    break;
	}
	case rightAlignedAbove:
	{
	    if ( itmgeom.rightToAtLeast( mCP(otherPos.right()) ) )
		mUpdated();

	    break;
	}

	case alignedWith:
	{
	    int malign = horAlign( lom );
	    int othalign = constr->other_->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( itmgeom.leftToAtLeast( mCP(itmgeom.left()+othalign-malign)) )
		mUpdated();

	    break;
	}

	case alignedBelow:
	{
	    if ( itmgeom.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    int malign = horAlign( lom );
	    int othalign = constr->other_->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( itmgeom.leftToAtLeast( mCP(itmgeom.left()+othalign-malign)) )
		mUpdated();

	    break;
	}

	case alignedAbove:
	{
	    int malign = horAlign( lom );
	    int othalign = constr->other_->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( itmgeom.leftToAtLeast( mCP(itmgeom.left()+othalign-malign)) )
		mUpdated();

	    break;
	}

	case centeredBelow:
	{
	    if ( itmgeom.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    if ( center(lom) > 0 && constr->other_->center(lom) > 0 &&
		itmgeom.leftToAtLeast( mCP(itmgeom.left()
				    + constr->other_->center(lom)
				    - center(lom))
				  )
	      )
		mUpdated();
	    break;
	}
	case centeredAbove:
	{
	    if ( center(lom) > 0 && constr->other_->center(lom) > 0 &&
		itmgeom.leftToAtLeast( mCP(itmgeom.left()
				    + constr->other_->center(lom)
				    - center(lom))
				  )
	      )
		mUpdated();
	    break;
	}

	case centeredLeftOf:
	{
	    if ( itmgeom.rightToAtLeast(mCP(otherPos.left() - mHorSpacing)))
		mUpdated();

	    if ( center(lom,false) > 0 &&
		 constr->other_->center(lom,false) > 0 &&
		 itmgeom.topToAtLeast( mCP(itmgeom.top()
				    + constr->other_->center(lom,false)
				    - center(lom,false))
				  )
	      )
		mUpdated();
	    break;
	}

	case centeredRightOf:
	{
	    if ( itmgeom.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated();

	    if ( center(lom,false) > 0 &&
		 constr->other_->center(lom,false) > 0 &&
		 itmgeom.topToAtLeast( mCP(itmgeom.top()
				    + constr->other_->center(lom,false)
				    - center(lom,false))
				  )
	      )
		mUpdated();
	    break;
	}


	case hCentered:
	{
	    if ( finalloop )
	    {
		int mngrcenter = ( mngr().curpos(lom).left()
				     + mngr().curpos(lom).right() ) / 2;

		const int shift = mngrcenter>=0 ?  mngrcenter - center(lom) : 0;
		if ( shift > 0 )
		{
		    if ( itmgeom.leftToAtLeast( mCP(itmgeom.left() + shift) ) )
			mUpdated();
		}
	    }
	    break;
	}


	case ensureRightOf:
	{
	    if ( itmgeom.leftToAtLeast( mCP(otherPos.right() + mHorSpacing )))
		mUpdated();

	    break;
	}
	case ensureBelow:
	{
	    if ( itmgeom.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing )))
		mUpdated();

	    break;
	}
	case leftBorder:
	{
	    if ( finalloop )
	    {
		int nwLeft = mngr().curpos(lom).left() + mInsideBorder;
		if ( itmgeom.left() != nwLeft )
		{
		    itmgeom.leftTo( mCP(nwLeft));
		    mUpdated();
		}
	    }
	    break;
	}
	case rightBorder:
	{
	    if ( finalloop )
	    {
		int nwRight = mngr().curpos(lom).right() - mInsideBorder;
		if ( itmgeom.right() != nwRight )
		{
		    itmgeom.rightTo( mCP(nwRight));
		    mUpdated();
		}
	    }
	    break;
	}
	case topBorder:
	{
	    if ( finalloop )
	    {
		int nwTop = mngr().curpos(lom).top() + mInsideBorder;
		if ( itmgeom.top() != nwTop )
		{
		    itmgeom.topTo( mCP(nwTop ));
		    mUpdated();
		}
	    }
	    break;
	}
	case bottomBorder:
	{
	    if ( finalloop )
	    {
		int nwBottom = mngr().curpos(lom).bottom()- mInsideBorder;
		if ( itmgeom.bottom() != nwBottom )
		{
		    itmgeom.bottomTo( mCP(nwBottom ));
		    mUpdated();
		}
	    }
	    break;
	}
	case heightSameAs:
	{
	    if ( itmgeom.vNrPics() < ( otherPos.vNrPics() ) )
	    {
		itmgeom.setVNrPics( otherPos.vNrPics() );
		mUpdated();
	    }
	    break;
	}
	case widthSameAs:
	{
	    if ( itmgeom.hNrPics() < ( otherPos.hNrPics() ) )
	    {
		itmgeom.setHNrPics( otherPos.hNrPics() );
		mUpdated();
	    }
	    break;
	}
	case stretchedBelow:
	{
	    int nwLeft = mFullStretch() ? mngr().winpos(lom).left()
					: mngr().curpos(lom).left();

	    if ( finalloop && itmgeom.left() != nwLeft )
	    {
		itmgeom.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).hNrPics()
					: mngr().curpos(lom).hNrPics();

	    if ( finalloop &&  itmgeom.hNrPics() < nwWidth )
	    {
		itmgeom.setHNrPics( nwWidth );
		mUpdated();
	    }
	    if ( itmgeom.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    break;
	}
	case stretchedAbove:
	{
	    int nwLeft = mFullStretch() ? mngr().winpos(lom).left()
					: mngr().curpos(lom).left();
	    if ( finalloop && itmgeom.left() != nwLeft )
	    {
		itmgeom.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).hNrPics()
					: mngr().curpos(lom).hNrPics();

	    if ( finalloop &&  itmgeom.hNrPics() < nwWidth )
	    {
		itmgeom.setHNrPics( nwWidth );
		mUpdated();
	    }

	    break;
	}
	case stretchedLeftTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top()
					: mngr().curpos(lom).top();
	    if ( finalloop && itmgeom.top() != nwTop )
	    {
		itmgeom.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).vNrPics()
					  : mngr().curpos(lom).vNrPics();
	    if ( finalloop && itmgeom.vNrPics() < nwHeight )
	    {
		itmgeom.setVNrPics( nwHeight );
		mUpdated();
	    }

	    break;
	}
	case stretchedRightTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top()
					: mngr().curpos(lom).top();
	    if ( finalloop && itmgeom.top() != nwTop )
	    {
		itmgeom.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).vNrPics()
					  : mngr().curpos(lom).vNrPics();
	    if ( finalloop && itmgeom.vNrPics() < nwHeight )
	    {
		itmgeom.setVNrPics( nwHeight );
		mUpdated();
	    }
	    if ( itmgeom.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated();

	    break;
	}
	case ensureLeftOf:
	{
	    break;
	}
	case atSamePosition:
	{
	    if ( itmgeom.topLeft() != otherPos.topLeft() )
	    {
		itmgeom.setTopLeft( otherPos.topLeft() );
		mUpdated();
	    }
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


void i_LayoutItem::attach( constraintType type, i_LayoutItem* other,
			   int margn, bool reciprocal )
{
    if ( type != ensureLeftOf )
	constraintlist_ += uiConstraint( type, other, margn );

    if ( reciprocal && other )
    {
	TypeSet<uiConstraint>& list = other->constraintlist_;
	switch ( type )
	{
	case leftOf:
	    list += uiConstraint( rightOf, this, margn );
	break;

	case rightOf:
	    list += uiConstraint( leftOf, this, margn );
	break;

	case leftTo:
	    list += uiConstraint( rightTo, this, margn );
	break;

	case rightTo:
	    list += uiConstraint( leftTo, this, margn );
	break;

	case leftAlignedBelow:
	    list += uiConstraint( leftAlignedAbove, this, margn );
	break;

	case leftAlignedAbove:
	    list += uiConstraint( leftAlignedBelow, this, margn );
	break;

	case rightAlignedBelow:
	    list += uiConstraint( rightAlignedAbove, this, margn );
	break;

	case rightAlignedAbove:
	    list += uiConstraint( rightAlignedBelow, this, margn );
	break;

	case alignedWith:
	    list += uiConstraint( alignedWith, this, margn );
	break;

	case alignedBelow:
	    list += uiConstraint( alignedAbove, this, margn );
	break;

	case alignedAbove:
	    list += uiConstraint( alignedBelow, this, margn );
	break;

	case centeredBelow:
	    list += uiConstraint( centeredAbove, this, margn );
	break;

	case centeredAbove:
	    list += uiConstraint( centeredBelow, this, margn );
	break;

	case centeredLeftOf:
	    list += uiConstraint( centeredRightOf, this, margn );
	break;

	case centeredRightOf:
	    list += uiConstraint( centeredLeftOf, this, margn );
	break;

	case heightSameAs:
	    vsameas_=true;
	break;

	case widthSameAs:
	    hsameas_=true;
	break;

	case ensureLeftOf:
	    list += uiConstraint( ensureRightOf, this, margn );
	break;

	case stretchedBelow:
	break;

	case stretchedAbove:
	    list += uiConstraint( ensureBelow, this, margn );
	break;

	case stretchedLeftTo:
	    list += uiConstraint( stretchedRightTo, this, margn );
	break;

	case stretchedRightTo:
	    list += uiConstraint( stretchedLeftTo, this, margn );
	break;

	case atSamePosition:
	    list += uiConstraint( atSamePosition, this, margn );
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
}


bool i_LayoutItem::isAligned() const
{
    for ( int idx=0; idx<constraintlist_.size(); idx++ )
    {
	constraintType tp = constraintlist_[idx].type_;
	if ( tp >= alignedWith && tp <= centeredAbove )
	    return true;
    }

    return false;
}


const uiObject* i_LayoutItem::objLayouted() const
{
    return const_cast<i_LayoutItem*>(this)->objLayouted();
}


const uiObjectBody* i_LayoutItem::bodyLayouted() const
{
    return const_cast<i_LayoutItem*>(this)->bodyLayouted();
}


QLayoutItem& i_LayoutItem::qlayoutItm()
{ return *qlayoutitm_; }


const QLayoutItem& i_LayoutItem::qlayoutItm() const
{ return *qlayoutitm_; }


QLayoutItem* i_LayoutItem::takeQlayoutItm()
{
    mQtclass(QLayoutItem*) ret = qlayoutitm_;
    qlayoutitm_ = nullptr;
    return ret;
}


const QWidget* i_LayoutItem::qwidget_() const
{
    return qlayoutitm_ ? qlayoutitm_->widget() : nullptr;
}


const QWidget* i_LayoutItem::managewidg_() const
{
    return qlayoutitm_ ? qlayoutitm_->widget() : nullptr;
}


// i_uiLayoutItem
i_uiLayoutItem::~i_uiLayoutItem()
{
    uiobjbody_.loitemDeleted();
}


uiSize i_uiLayoutItem::minimumSize() const
{
    uiSize s = uiobjbody_.minimumSize();
    if ( !mIsUdf(s.hNrPics()) )
	return s;

    return i_LayoutItem::minimumSize();
}
