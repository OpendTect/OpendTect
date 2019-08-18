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

i_LayoutItem::i_LayoutItem( i_LayoutMngr& m, QLayoutItem& itm )
    : mngr_(m), qlayoutitm_(&itm)
    , preferred_pos_inited_(false), minimum_pos_inited_(false)
    , prefszdone_(false), hsameas_(false), vsameas_(false)
{
#ifdef __debug__
    mDefineStaticLocalObject( bool, lyoutdbg_loc,
			      = GetEnvVarYN("DTECT_DEBUG_LAYOUT") );
    lyoutdbg = lyoutdbg_loc;
#endif
}

i_LayoutItem::~i_LayoutItem()
{
    sendDelNotif();
    delete qlayoutitm_;
}


int i_LayoutItem::horAlign( LayoutMode m ) const
{
    return curpos(m).left();
}


int i_LayoutItem::centre( LayoutMode m, bool hor ) const
{
    return hor ? ( curpos(m).left() + curpos(m).right() ) / 2
	       : ( curpos(m).top() + curpos(m).bottom() ) / 2;
}


uiSize i_LayoutItem::minimumSize() const
{
    QSize s = qwidget()->minimumSize();
    return uiSize( s.width(), s.height() );
}


uiSize i_LayoutItem::prefSize() const
{
    if ( prefszdone_ )
       { pErrMsg("PrefSize already done.");}
    else
    {
	i_LayoutItem* self = const_cast<i_LayoutItem*>(this);
	self->prefszdone_ = true;
	QSize ps( qlayoutItm().sizeHint() );
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


const uiRect& i_LayoutItem::curpos( LayoutMode m ) const
{ return layoutpos_[m]; }

uiRect& i_LayoutItem::curpos( LayoutMode m )
{ return layoutpos_[m]; }


bool i_LayoutItem::inited() const
{
    return minimum_pos_inited_ || preferred_pos_inited_;
}


int i_LayoutItem::stretch( bool hor ) const
{
    if ( (hor && hsameas_) || (!hor && vsameas_) ) return 0;

    const uiObjectBody* blo = bodyLayouted();
    return blo ? blo->stretch( hor ) : 0;
}


void i_LayoutItem::commitGeometrySet( bool store2prefpos )
{
    uiRect mPos = curpos( setGeom );

    if ( store2prefpos ) curpos( preferred ) = mPos;

    if ( objLayouted() ) objLayouted()->triggerSetGeometry( this, mPos );
#ifdef __debug__
    if ( lyoutdbg )
    {
	od_cout() << "Setting layout on: ";
	if( objLayouted() )
	    od_cout() << objLayouted()->name() << od_endl;
	else
	    od_cout() << "Unknown" << od_endl;

	od_cout() << "l: " << mPos.left() << " t: " << mPos.top();
	od_cout() << " hor: " << mPos.hNrPics() << " ver: "
			 << mPos.vNrPics() << od_endl;
    }
#endif

    qlayoutitm_->setGeometry(
	QRect(mPos.left(),mPos.top(),mPos.hNrPics(),mPos.vNrPics()) );
}


void i_LayoutItem::initLayout( LayoutMode lom, int mngrTop, int mngrLeft )
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
	    pref_h_nr_pics	= prefSize().hNrPics();
	    pref_v_nr_pics	= prefSize().vNrPics();
	}
    }

#ifdef __debug__
    if ( lyoutdbg )
    {
	BufferString blnm = bodyLayouted() ?  bodyLayouted()->name().buf()
					   : "";

	od_cout() << "Init layout on:" << blnm;
	od_cout() << ": prf hsz: " << pref_h_nr_pics;
	od_cout() <<",  prf vsz: " << pref_v_nr_pics;
	od_cout() <<", mngr top: " << mngrTop;
	od_cout() <<", mngr left: " << mngrLeft;
	od_cout() <<",  layout mode: " << (int) lom << od_endl;
    }
#endif

    switch ( lom )
    {
	case minimum:
	    if ( !minimum_pos_inited_)
	    {
		mPos.zero();
		uiSize ms = minimumSize();
		mPos.setHNrPics( ms.hNrPics() );
		mPos.setVNrPics( ms.vNrPics() );
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
		uiRect& mPos2 = curpos( lom );
		mPos2 = curpos( preferred );

		mPos2.leftTo( mMAX( pPos.left(), mngrLeft ));
		mPos2.topTo( mMAX( pPos.top(), mngrTop ));

		initChildLayout(lom);
	    }
	    break;

	case preferred:
	    {
		mPos.setLeft( mngrLeft );
		mPos.setTop( mngrTop );

		mPos.setHNrPics( pref_h_nr_pics  );
		mPos.setVNrPics( pref_v_nr_pics );
		preferred_pos_inited_ = true;
	    }
	    break;
	case all:
	    break;
    }

    if ( mPos.left() < 0 )
	{ pErrMsg("left < 0"); }
    if ( mPos.top() < 0 )
	{ pErrMsg("top < 0"); }

}


#ifdef __debug__

int i_LayoutItem::isPosOk( uiConstraint* constraint, int iter, bool chknriters )
{
    if ( (chknriters && iter<MAX_ITER) || iter <= 2000 )
	return iter;

    if ( constraint->enabled() )
    {
	BufferString msg( chknriters ? "Too many iterations: "
				     : "Layout loop: " );
	if ( objLayouted() )
	    msg.add( "'" ).add( objLayouted()->name() ).add( "' " );

	switch ( constraint->type_ )
	{
#	    define mCase(typ) case typ: msg.add( #typ ); break

	    mCase( leftOf );
	    mCase( rightOf );
	    mCase( leftTo );
	    mCase( rightTo );
	    mCase( leftAlignedBelow );
	    mCase( leftAlignedAbove );
	    mCase( rightAlignedBelow );
	    mCase( rightAlignedAbove );
	    mCase( alignedBelow );
	    mCase( alignedAbove );
	    mCase( centeredBelow );
	    mCase( centeredAbove );
	    mCase( ensureLeftOf );
	    mCase( ensureRightOf );
	    mCase( ensureBelow );
	    mCase( leftBorder );
	    mCase( rightBorder );
	    mCase( topBorder );
	    mCase( bottomBorder );
	    mCase( heightSameAs );
	    mCase( widthSameAs );
	    mCase( stretchedBelow );
	    mCase( stretchedAbove );
	    mCase( stretchedLeftTo );
	    mCase( stretchedRightTo );
	    mCase( atSamePosition );

	    default:	msg.add( "<new layout type?>" ); break;
	}

	if ( constraint->other_ && constraint->other_->objLayouted() )
	    msg.add( " '" ).add( constraint->other_->objLayouted()->name() )
		.add( "'" );
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


#define mHorSpacing (constr->margin_>=0 ? constr->margin_ : mngr_.horSpacing())
#define mVerSpacing (constr->margin_>=0 ? constr->margin_ : mngr_.verSpacing())

#define mFullStretch() (constr->margin_ < -1)
#define mInsideBorder  (constr->margin_ > mngr_.borderSpace() \
			 ? constr->margin_ - mngr_.borderSpace() : 0)

bool i_LayoutItem::layout( LayoutMode lom, int iternr, bool finalloop )
{
    bool updated = false;
    uiRect& mPos = curpos(lom);

    for ( int idx=0; idx<constrlist_.size(); idx++ )
    {
	uiConstraint* constr = &constrlist_[idx];
	const uiRect& otherPos
		= constr->other_ ? constr->other_->curpos(lom) : curpos(lom);

	switch ( constr->type_ )
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

	case alignedWith:
	{
	    int malign = horAlign( lom );
	    int othalign = constr->other_->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) )
		mUpdated();

	    break;
	}

	case alignedBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    int malign = horAlign( lom );
	    int othalign = constr->other_->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) )
		mUpdated();

	    break;
	}

	case alignedAbove:
	{
	    int malign = horAlign( lom );
	    int othalign = constr->other_->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) )
		mUpdated();

	    break;
	}

	case centeredBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    if ( centre(lom) > 0 && constr->other_->centre(lom) > 0 &&
		mPos.leftToAtLeast( mCP(mPos.left()
				    + constr->other_->centre(lom)
				    - centre(lom))
				  )
	      )
		mUpdated();
	    break;
	}
	case centeredAbove:
	{
	    if ( centre(lom) > 0 && constr->other_->centre(lom) > 0 &&
		mPos.leftToAtLeast( mCP(mPos.left()
				    + constr->other_->centre(lom)
				    - centre(lom))
				  )
	      )
		mUpdated();
	    break;
	}

	case centeredLeftOf:
	{
	    if ( mPos.rightToAtLeast(mCP(otherPos.left() - mHorSpacing)))
		mUpdated();

	    if ( centre(lom,false) > 0 &&
		 constr->other_->centre(lom,false) > 0 &&
		 mPos.topToAtLeast( mCP(mPos.top()
				    + constr->other_->centre(lom,false)
				    - centre(lom,false))
				  )
	      )
		mUpdated();
	    break;
	}

	case centeredRightOf:
	{
	    if ( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated();

	    if ( centre(lom,false) > 0 &&
		 constr->other_->centre(lom,false) > 0 &&
		 mPos.topToAtLeast( mCP(mPos.top()
				    + constr->other_->centre(lom,false)
				    - centre(lom,false))
				  )
	      )
		mUpdated();
	    break;
	}


	case hCentered:
	{
	    if ( finalloop )
	    {
		int mngrcentre = ( mngr().curpos(lom).left()
				     + mngr().curpos(lom).right() ) / 2;

		int shift = mngrcentre >= 0 ?  mngrcentre - centre(lom) : 0;

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
	    if ( finalloop )
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
	    if ( finalloop )
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
	    if ( finalloop )
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
	    if ( finalloop )
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
	    if ( mPos.vNrPics() < ( otherPos.vNrPics() ) )
	    {
		mPos.setVNrPics( otherPos.vNrPics() );
		mUpdated();
	    }
	    break;
	}
	case widthSameAs:
	{
	    if ( mPos.hNrPics() < ( otherPos.hNrPics() ) )
	    {
		mPos.setHNrPics( otherPos.hNrPics() );
		mUpdated();
	    }
	    break;
	}
	case stretchedBelow:
	{
	    int nwLeft = mFullStretch() ? mngr().winpos(lom).left()
					: mngr().curpos(lom).left();

	    if ( finalloop && mPos.left() != nwLeft )
	    {
		mPos.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).hNrPics()
					: mngr().curpos(lom).hNrPics();

	    if ( finalloop &&  mPos.hNrPics() < nwWidth )
	    {
		mPos.setHNrPics( nwWidth );
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
	    if ( finalloop && mPos.left() != nwLeft )
	    {
		mPos.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).hNrPics()
					: mngr().curpos(lom).hNrPics();

	    if ( finalloop &&  mPos.hNrPics() < nwWidth )
	    {
		mPos.setHNrPics( nwWidth );
		mUpdated();
	    }

	    break;
	}
	case stretchedLeftTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top()
					: mngr().curpos(lom).top();
	    if ( finalloop && mPos.top() != nwTop )
	    {
		mPos.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).vNrPics()
					  : mngr().curpos(lom).vNrPics();
	    if ( finalloop && mPos.vNrPics() < nwHeight )
	    {
		mPos.setVNrPics( nwHeight );
		mUpdated();
	    }

	    break;
	}
	case stretchedRightTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top()
					: mngr().curpos(lom).top();
	    if ( finalloop && mPos.top() != nwTop )
	    {
		mPos.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).vNrPics()
					  : mngr().curpos(lom).vNrPics();
	    if ( finalloop && mPos.vNrPics() < nwHeight )
	    {
		mPos.setVNrPics( nwHeight );
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
	case atSamePosition:
	{
	    if ( mPos.topLeft() != otherPos.topLeft() )
	    {
		mPos.setTopLeft( otherPos.topLeft() );
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


void i_LayoutItem::attach ( ConstraintType type, i_LayoutItem* other,
			    int margn, bool reciprocal )
{
    if ( type != ensureLeftOf)
	constrlist_ += uiConstraint( type, other, margn );

    if( reciprocal && other )
    {
	switch ( type )
	{
	case leftOf:
	    other->constrlist_ += uiConstraint( rightOf, this, margn );
	break;

	case rightOf:
	    other->constrlist_ += uiConstraint( leftOf, this, margn );
	break;

	case leftTo:
	    other->constrlist_ += uiConstraint( rightTo, this, margn );
	break;

	case rightTo:
	    other->constrlist_ += uiConstraint( leftTo, this, margn );
	break;

	case leftAlignedBelow:
	    other->constrlist_ += uiConstraint( leftAlignedAbove, this, margn );
	break;

	case leftAlignedAbove:
	    other->constrlist_ += uiConstraint( leftAlignedBelow, this, margn );
	break;

	case rightAlignedBelow:
	    other->constrlist_ += uiConstraint( rightAlignedAbove, this, margn);
	break;

	case rightAlignedAbove:
	    other->constrlist_ += uiConstraint( rightAlignedBelow, this, margn);
	break;

	case alignedWith:
	    other->constrlist_ += uiConstraint( alignedWith, this, margn );
	break;

	case alignedBelow:
	    other->constrlist_ += uiConstraint( alignedAbove, this, margn );
	break;

	case alignedAbove:
	    other->constrlist_ += uiConstraint( alignedBelow, this, margn );
	break;

	case centeredBelow:
	    other->constrlist_ += uiConstraint( centeredAbove, this, margn );
	break;

	case centeredAbove:
	    other->constrlist_ += uiConstraint( centeredBelow, this, margn );
	break;

	case centeredLeftOf:
	    other->constrlist_ += uiConstraint( centeredRightOf, this, margn );
	break;

	case centeredRightOf:
	    other->constrlist_ += uiConstraint( centeredLeftOf, this, margn );
	break;

	case heightSameAs:
	    vsameas_=true;
	break;

	case widthSameAs:
	    hsameas_=true;
	break;

	case ensureLeftOf:
	    other->constrlist_ += uiConstraint( ensureRightOf, this, margn );
	break;

	case stretchedBelow:
	break;

	case stretchedAbove:
	    other->constrlist_ += uiConstraint( ensureBelow, this, margn );
	break;

	case stretchedLeftTo:
	    other->constrlist_ += uiConstraint( stretchedRightTo, this, margn );
	break;

	case stretchedRightTo:
	    other->constrlist_ += uiConstraint( stretchedLeftTo, this, margn );
	break;

	case atSamePosition:
	    other->constrlist_ += uiConstraint( atSamePosition, this, margn );
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


const uiObject* i_LayoutItem::objLayouted() const
{
    return const_cast<i_LayoutItem*>(this)->objLayouted();
}


const uiObjectBody* i_LayoutItem::bodyLayouted() const
{
    return const_cast<i_LayoutItem*> (this)->bodyLayouted();
}


QLayoutItem& i_LayoutItem::qlayoutItm()
{ return *qlayoutitm_; }

const QLayoutItem& i_LayoutItem::qlayoutItm() const
{ return *qlayoutitm_; }


QLayoutItem* i_LayoutItem::takeQlayoutItm()
{
    QLayoutItem* ret = qlayoutitm_;
    qlayoutitm_ = 0;
    return ret;
}


const QWidget* i_LayoutItem::qwidget_() const
{ return qlayoutitm_->widget(); }

const QWidget* i_LayoutItem::managewidg_() const
{ return qlayoutitm_->widget(); }


bool i_LayoutItem::isAligned() const
{
    for ( int idx=0; idx<constrlist_.size(); idx++ )
    {
	ConstraintType tp = constrlist_[idx].type_;
	if ( tp >= alignedWith && tp <= centeredAbove )
	    return true;
    }

    return false;
}


// i_uiLayoutItem
i_uiLayoutItem::i_uiLayoutItem( i_LayoutMngr& mgr, uiObjectBody& obj )
    : i_LayoutItem(mgr,*new QWidgetItem(obj.qwidget()) )
, uiobjbody_(obj)
{}


i_uiLayoutItem::~i_uiLayoutItem()
{
    uiobjbody_.loitemDeleted();
}


uiSize i_uiLayoutItem::minimumSize() const
{
    uiSize sz = uiobjbody_.minimumSize();
    if ( !mIsUdf(sz.hNrPics()) )
	return sz;

    return i_LayoutItem::minimumSize();
}


uiObject* i_uiLayoutItem::objLayouted()
{ return &uiobjbody_.uiObjHandle(); }


uiObjectBody* i_uiLayoutItem::bodyLayouted()
{ return &uiobjbody_; }
