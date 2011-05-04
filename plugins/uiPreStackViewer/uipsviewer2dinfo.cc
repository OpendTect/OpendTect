/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2dinfo.cc,v 1.1 2011-05-04 15:20:02 cvsbruno Exp $";

#include "uilabel.h"
#include "uipsviewer2dinfo.h"

namespace PreStackView
{

uiGatherDisplayInfoHeader::uiGatherDisplayInfoHeader( uiParent* p ) 
    : uiGroup( p, "Pre-stack gather Display Info Header" )
{
    setStretch( 2, 2 );
    datalbl_ = new uiLabel(this,"");
    poslbl_ = new uiLabel(this,"");

    poslbl_->setPrefWidthInChar( 10 );
    datalbl_->setPrefWidthInChar( 30 );

    datalbl_->attach( ensureBelow, poslbl_ );
    datalbl_->setPrefHeight( 20 );
    poslbl_->setPrefHeight( 20 );

    setVSpacing( 0 );
}


void uiGatherDisplayInfoHeader::setOffsetRange( const Interval<float>& offs )
{
    //TODO display axis range in a graphcis view
}


void uiGatherDisplayInfoHeader::setData( const BinID& pos, bool isinl, 
					const char* datanm )
{
    datalbl_->setText( datanm ); 
    BufferString posstr( isinl ? "Crl " : "Inl " ); 
    posstr += isinl ? toString( pos.crl ) : toString( pos.inl );
    poslbl_->setText( posstr.buf() );
}



} //namespace
