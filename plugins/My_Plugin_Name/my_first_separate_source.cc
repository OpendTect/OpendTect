/*+
 * (C) Your_copyright
 * AUTHOR   : Your_name_here
 * DATE     : Apr 2012
-*/

static const char* mUnusedVar rcsID = "$Id: my_first_separate_source.cc,v 1.2 2012-05-02 11:52:47 cvskris Exp $";

#include "my_first_separate_source.h"
#include "uimsg.h"


My_Class::My_Class()
    : my_variable_(0)
{
    uiMSG().message( "Hello world!" );
}
