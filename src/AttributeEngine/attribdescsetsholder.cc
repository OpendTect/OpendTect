/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. HUCK
 * DATE     : Mar 2010
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "attribdescsetsholder.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "survinfo.h"

namespace Attrib
{

DescSetsHolder* DescSetsHolder::dsholder_ = 0;

const DescSetsHolder& DSHolder()
{
    if ( !DescSetsHolder::dsholder_ )
	DescSetsHolder::dsholder_ = new DescSetsHolder();

    return *DescSetsHolder::dsholder_;
}


DescSetsHolder& eDSHolder()
{
    return const_cast<DescSetsHolder&>( DSHolder() );
}


DescSetsHolder::DescSetsHolder()
{
    adsman2d_ = new DescSetMan( true );
    adsman3d_ = new DescSetMan( false );
    adsstored2d_ = new DescSet( true );
    adsstored3d_ = new DescSet( false );

    adsstored2d_->setContainStoredDescOnly( true );
    adsstored3d_->setContainStoredDescOnly( true );
}


DescSetsHolder::~DescSetsHolder()
{
    clearHolder();
    dsholder_ = 0;
}


DescSetMan* DescSetsHolder::getDescSetMan( bool is2d )
{
    return is2d ? adsman2d_ : adsman3d_;
}


const DescSetMan* DescSetsHolder::getDescSetMan( bool is2d ) const
{
    return is2d ? adsman2d_ : adsman3d_;
}


DescSet* DescSetsHolder::getDescSet( bool is2d, bool isstored )
{
    return is2d ? isstored ? adsstored2d_ : adsman2d_->descSet()
		: isstored ? adsstored3d_ : adsman3d_->descSet();
}


const DescSet* DescSetsHolder::getDescSet( bool is2d, bool isstored ) const
{
    return is2d ? isstored ? adsstored2d_ : adsman2d_->descSet()
		: isstored ? adsstored3d_ : adsman3d_->descSet();
}


void DescSetsHolder::clearHolder()
{
    delete adsman2d_;
    delete adsman3d_;
    delete adsstored2d_;
    delete adsstored3d_;
}


void DescSetsHolder::replaceAttribSet( DescSet* ads )
{
    bool is2d = ads && ads->is2D();
    if ( is2d )
    {
	delete adsman2d_;
	adsman2d_ = new DescSetMan( is2d, ads, true );
    }
    else
    {
	delete adsman3d_;
	adsman3d_ = new DescSetMan( is2d, ads, true );
    }
}


void DescSetsHolder::replaceADSMan( DescSetMan* newadsman, bool dodelete )
{
    if ( !newadsman ) return;
    if ( newadsman->is2D() )
    {
	if ( dodelete ) delete adsman2d_;
	adsman2d_ = newadsman;
    }
    else
    {
	if ( dodelete ) delete adsman3d_;
	adsman3d_ = newadsman;
    }
}


void DescSetsHolder::replaceStoredAttribSet( DescSet* ads )
{
    bool is2d = ads && ads->is2D();
    if ( is2d )
    {
	delete adsstored2d_;
	adsstored2d_ = ads;
    }
    else
    {
	delete adsstored3d_;
	adsstored3d_ = ads;
    }
}

}; // namespace Attrib
