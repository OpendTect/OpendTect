/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "useraction.h"

UserAction::Setup::Setup()
    : checkable_( false ), checked_( false ), enabled_( true ), visible_( true )
{}


UserAction::UserAction( const UserAction& ua )
    : setup_( ua.setup_ )
    , id_( -1 )
    , change_( this )
{}
