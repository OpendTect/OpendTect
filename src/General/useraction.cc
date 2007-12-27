/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2007
 RCS:           $Id: useraction.cc,v 1.1 2007-12-27 15:59:46 cvskris Exp $
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
