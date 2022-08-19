/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "useraction.h"

UserAction::Setup::Setup()
    : checkable_( false ), checked_( false ), enabled_( true ), visible_( true )
{}


UserAction::UserAction( const UserAction& ua )
    : setup_( ua.setup_ )
    , id_( -1 )
    , change_( this )
{}
