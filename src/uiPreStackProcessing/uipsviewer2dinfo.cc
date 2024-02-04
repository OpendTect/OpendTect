/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


uiGatherDisplayInfoHeader::~uiGatherDisplayInfoHeader()
{}


void uiGatherDisplayInfoHeader::setOffsetRange( const Interval<float>& offs )
{
    //TODO display axis range in a graphcis view
}


void uiGatherDisplayInfoHeader::setData( const TrcKey& tk, bool isinl,
					 bool  is2d, const char* datanm )
{
    datalbl_->setText( toUiString(datanm) );
    if ( tk.isUdf() )
	return;

    uiString posstr = toUiString("%1 %2").arg(is2d ? uiStrings::sTrace() :
					  isinl ? tr("Crl ") : tr("Inl "))
					  .arg(isinl ? toUiString( tk.crl() )
						     : toUiString( tk.inl() ));
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

} // namespace PreStackView
