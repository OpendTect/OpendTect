/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
-*/


#include "attribfactory.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "msgh.h"

namespace Attrib
{

ProviderFactory::ProviderFactory()
{}


ProviderFactory::~ProviderFactory()
{
    for ( int idx=0; idx<descs_.size(); idx++ )
	descs_[idx]->unRef();

    descs_.erase();
    creaters_.erase();
}


void ProviderFactory::addDesc( Desc* nps, ProviderCreater pc )
{
    const int idx = indexOf(nps->attribName());
    if ( idx!=-1 )
	return;

    nps->ref();
    descs_ += nps;
    creaters_ += pc;
}


Provider* ProviderFactory::create( Desc& desc, uiRetVal& uirv,
				   bool skipchecks ) const
{
    if ( !skipchecks )
    {
	const auto lvl = desc.satisfyLevel();
	if ( Desc::isError(lvl) )
	{
	    if ( lvl != Desc::StorNotFound )
		uirv = tr("Error in definition of %1 attribute.")
			 .arg( desc.attribName() );
	    else
	    {
		uirv = tr("Impossible to find stored data '%1'\n"
				 "used as input for other attribute(s). \n"
				 "Data might have been deleted or corrupted.\n"
				 "Please check your attribute set \n"
				 "Please select valid stored data.")
				.arg( desc.userRef() );
	    }
	    return nullptr;
	}

    }

    const int idx = indexOf( desc.attribName() );
    if ( idx < 0 )
    {
	uirv = mINTERNAL( "Attribute not in factory" );
	return nullptr;
    }

    return creaters_[idx]( desc );
}


const Desc* ProviderFactory::getDesc( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : descs_[idx];
}


Desc* ProviderFactory::createDescCopy( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : new Desc( *descs_[idx] );
}


int ProviderFactory::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	if ( descs_[idx]->attribName()==nm )
	    return idx;
    }

    return -1;
}


void ProviderFactory::updateAllDescsDefaults()
{
    for ( int idx=0; idx<descs_.size(); idx++ )
	descs_[idx]->updateDefaultParams();
}


ProviderFactory& PF()
{
    mDefineStaticLocalObject(PtrMan<ProviderFactory>, factory,
                             (new ProviderFactory));
    return *factory;
}

} // namespace Attrib
