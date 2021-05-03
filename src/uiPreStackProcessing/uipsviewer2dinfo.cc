/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/

#include "uilabel.h"
#include "uipsviewer2dinfo.h"
#include "uistrings.h"

namespace PreStackView
{

uiGatherDisplayInfoHeader::uiGatherDisplayInfoHeader( uiParent* p )
    : uiGroup( p, "Prestack gather Display Info Header" )
{
    setStretch( 2, 2 );
    datalbl_ = new uiLabel(this,uiStrings::sEmptyString());
    datalbl_->setVSzPol( uiObject::Small );
    poslbl_ = new uiLabel(this,uiStrings::sEmptyString());
    poslbl_->setVSzPol( uiObject::Small );

    poslbl_->setPrefWidthInChar( 30 );
    datalbl_->setPrefWidthInChar( 30 );
    poslbl_->setStretch(2,2);
    datalbl_->setStretch(2,2);

    datalbl_->attach( ensureBelow, poslbl_ );

    setVSpacing( 0 );
}


void uiGatherDisplayInfoHeader::setOffsetRange( const Interval<float>& offs )
{
    //TODO display axis range in a graphcis view
}


void uiGatherDisplayInfoHeader::setData( const BinID& pos, bool isinl,
					bool  is2d, const char* datanm )
{
    datalbl_->setText( toUiString(datanm) );
    if ( mIsUdf(pos.inl()) && mIsUdf(pos.crl()) )
	return;

    uiString posstr = toUiString("%1 %2").arg(is2d ? uiStrings::sTrace() : 
					  isinl ? tr("Crl ") : tr("Inl "))
					  .arg(isinl ? toUiString( pos.crl() ) 
						     : toUiString( pos.inl() ));
    poslbl_->setText( posstr );
}


void uiGatherDisplayInfoHeader::setData( int pos, const char* datanm )
{
    datalbl_->setText( toUiString(datanm) );
    uiString posstr = tr( "Model %1" ).arg(pos);
    poslbl_->setText( posstr );
}

const char* uiGatherDisplayInfoHeader::getDataName() const
{ return datalbl_->text().getFullString(); }

} //namespace
