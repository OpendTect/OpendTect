/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/02/2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "i_layoutitem.h"
#include "i_layout.h"

#include "uilayout.h"
#include "uimainwin.h"
#include "uiobjbody.h"

#include "envvars.h"
#include "errh.h"

#include <iostream>

#ifdef __debug__
# define MAX_ITER	2000
static bool lyoutdbg = GetEnvVarYN("DTECT_DEBUG_LAYOUT");
#else
# define MAX_ITER	10000
#endif

mUseQtnamespace

//------------------------------------------------------------------------------

i_LayoutItem::i_LayoutItem( i_LayoutMngr& m, QLayoutItem& itm ) 
    : mngr_( m ), qlayoutitm( &itm ) 
    , preferred_pos_inited( false ), minimum_pos_inited( false )
    , prefSzDone( false ), hsameas( false ), vsameas( false )
{
}

i_LayoutItem::~i_LayoutItem()
{
    delete qlayoutitm;
}


i_uiLayoutItem::~i_uiLayoutItem()
{
    uiObjBody_.loitemDeleted();
}


void i_LayoutItem::invalidate() 
{ 
    qlayoutitm->invalidate();
    preferred_pos_inited = false;
}


uiSize i_LayoutItem::actualsize( bool include_border ) const
{ 
    return curpos(setGeom).getPixelSize(); 
}


int i_LayoutItem::stretch( bool hor ) const
{ 
    if ( (hor && hsameas) || (!hor && vsameas) ) return 0;

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
	std::cout << "Setting layout on: ";
	if( objLayouted() ) 
	    std::cout << objLayouted()->name() << std::endl;
	else 
	    std::cout << "Unknown" << std::endl;

	std::cout << "l: " << mPos.left() << " t: " << mPos.top();
	std::cout << " hor: " << mPos.hNrPics() << " ver: "
	    		 << mPos.vNrPics() << std::endl;
    }
#endif

    qlayoutitm->setGeometry ( QRect ( mPos.left(), mPos.top(), 
                                      mPos.hNrPics(), mPos.vNrPics() )); 
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
	BufferString blnm = bodyLayouted() ?  bodyLayouted()->name() : "";

	std::cout << "Init layout on:" << blnm;
	std::cout << ": prf hsz: " << pref_h_nr_pics;
	std::cout <<",  prf vsz: " << pref_v_nr_pics;
	std::cout <<", mngr top: " << mngrTop;
	std::cout <<", mngr left: " << mngrLeft;
	std::cout <<",  layout mode: " << (int) lom << std::endl;
    }
#endif

    switch ( lom )
    {
	case minimum:
            if ( !minimum_pos_inited)
	    {
		mPos.zero();
		uiSize ms = minimumsize();
		mPos.setHNrPics( ms.hNrPics() );
		mPos.setVNrPics( ms.vNrPics() );
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
		preferred_pos_inited = true;
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
	BufferString msg;
	if ( chknriters )
	    msg = "\n  Too many iterations with: \"";
	else
	    msg = "\n  Layout loop on: \"";
	msg += objLayouted() ? (const char*)objLayouted()->name() : "UNKNOWN";
	msg += "\"";

	switch ( constraint->type_ )
	{
	    case leftOf: 		msg += " leftOf "; break;
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
	    case heightSameAs: 		msg += " heightSameAs "; break;
	    case widthSameAs:		msg += " widthSameAs "; break;
	    case stretchedBelow:	msg += " stretchedBelow "; break;
	    case stretchedAbove:	msg += " stretchedAbove "; break;
	    case stretchedLeftTo:	msg += " stretchedLeftTo "; break;
	    case stretchedRightTo:	msg += " stretchedRightTo "; break;
	    default:		 	msg += " .. "; break;
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


#define mHorSpacing (constr->margin_ >= 0 ? constr->margin_ : mngr_.horSpacing())
#define mVerSpacing (constr->margin_ >= 0 ? constr->margin_ : mngr_.verSpacing())

#define mFullStretch() (constr->margin_ < -1)
#define mInsideBorder  (constr->margin_ > mngr_.borderSpace() \
			 ? constr->margin_ - mngr_.borderSpace() : 0)

bool i_LayoutItem::layout( LayoutMode lom, const int iternr, bool finalloop )
{
    bool updated = false;
    uiRect& mPos = curpos(lom);

    for ( int idx=0; idx<constrList.size(); idx++ )
    {
	uiConstraint* constr = &constrList[idx];
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
	default:
	{
	    pErrMsg("Unknown constraint type");
	    break;
	}
	}
    }

    return updated;
}


void i_LayoutItem::attach ( constraintType type, i_LayoutItem* other, 
			    int margn, bool reciprocal )
{
    if ( type != ensureLeftOf)
	constrList += uiConstraint( type, other, margn );

    if( reciprocal && other )
    {
	switch ( type )
	{
	case leftOf:
	    other->constrList += uiConstraint( rightOf, this, margn );
	break;

	case rightOf:
	    other->constrList += uiConstraint( leftOf, this, margn );
	break;

	case leftTo:
	    other->constrList += uiConstraint( rightTo, this, margn);
	break;

	case rightTo:
	    other->constrList += uiConstraint( leftTo, this, margn );
	break;

	case leftAlignedBelow:
	    other->constrList += uiConstraint( leftAlignedAbove, this, margn );
	break;

	case leftAlignedAbove:
	    other->constrList += uiConstraint( leftAlignedBelow, this, margn );
	break;

	case rightAlignedBelow:
	    other->constrList += uiConstraint( rightAlignedAbove, this, margn );
	break;

	case rightAlignedAbove:
	    other->constrList += uiConstraint( rightAlignedBelow, this, margn );
	break;

	case alignedWith:
	    other->constrList += uiConstraint( alignedWith, this, margn );
	break;

	case alignedBelow:
	    other->constrList += uiConstraint( alignedAbove, this, margn );
	break;

	case alignedAbove:
	    other->constrList += uiConstraint( alignedBelow, this, margn );
	break;

	case centeredBelow:
	    other->constrList += uiConstraint( centeredAbove, this, margn );
	break;

	case centeredAbove:
	    other->constrList += uiConstraint( centeredBelow, this, margn );
	break;

	case centeredLeftOf:
	    other->constrList += uiConstraint( centeredRightOf, this, margn );
	break;

	case centeredRightOf:
	    other->constrList += uiConstraint( centeredLeftOf, this, margn );
	break;

	case heightSameAs:
	    vsameas=true;
	break;

	case widthSameAs:
	    hsameas=true;
	break;

	case ensureLeftOf:
	    other->constrList += uiConstraint( ensureRightOf, this, margn );
	break;

	case stretchedBelow:
	break;

	case stretchedAbove:
	    other->constrList += uiConstraint( ensureBelow, this, margn );
	break;

	case stretchedLeftTo:
	    other->constrList += uiConstraint( stretchedRightTo, this, margn );
	break;

	case stretchedRightTo:
	    other->constrList += uiConstraint( stretchedLeftTo, this, margn );
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
    for ( int idx=0; idx<constrList.size(); idx++ )
    { 
	constraintType tp = constrList[idx].type_;
	if ( tp >= alignedWith && tp <= centeredAbove )
	    return true; 
    }

    return false;
}
