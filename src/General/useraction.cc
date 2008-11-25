/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: useraction.cc,v 1.2 2008-11-25 15:35:22 cvsbert Exp $";


#include "useraction.h"

UserAction::Setup::Setup()
    : checkable_( false ), checked_( false ), enabled_( true ), visible_( true )
{}


UserAction::UserAction( const UserAction& ua )
    : setup_( ua.setup_ )
    , id_( -1 )
    , change_( this )
{}
