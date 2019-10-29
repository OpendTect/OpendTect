/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : September 2019
-*/

#include "procdescdata.h"
#include "uistring.h"


mDefineEnumUtils(ProcDesc::DataEntry,Type,"Type")
{ "OD", "Python", 0 };

template <>
void EnumDefImpl<ProcDesc::DataEntry::Type>::init()
 {
     uistrings_ += ::toUiString( "OD" );
     uistrings_ += ::toUiString( "Python" );
 }


ProcDesc::Data& ePDD()
{
    mDefineStaticLocalObject( PtrMan<ProcDesc::Data>, theinstance, = nullptr );
    return *theinstance.createIfNull();
}


const ProcDesc::Data& PDD()
{
    return ePDD();
}


ProcDesc::Data& ProcDesc::Data::add( ProcDesc::DataEntry* pdde )
{
    *this += pdde;
    return *this;
}


ProcDesc::Data& ProcDesc::Data::add( const char* nm, const uiString& desc,
						ProcDesc::DataEntry::Type typ )
{
    ProcDesc::DataEntry* pdde = new ProcDesc::DataEntry();
    pdde->desc_ = desc;
    pdde->execnm_ = nm;
    pdde->type_ = typ;

    return add( pdde );
}

#define mGetData(type) \

void ProcDesc::Data::getProcData(BufferStringSet& nms, uiStringSet& descs,
    DataEntry::Type type)
{
    for ( int idx=0; idx<ePDD().size(); idx++ )
    {
	ProcDesc::DataEntry* pdde = ePDD()[idx];
	if ( pdde->type_ == type )
	{
	    nms.add( pdde->execnm_ );
	    descs.add( pdde->desc_ );
	}
    }
}
