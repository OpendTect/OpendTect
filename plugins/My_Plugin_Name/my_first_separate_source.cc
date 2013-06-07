/*+
 * (C) Your_copyright
 * AUTHOR   : Your_name_here
 * DATE     : Apr 2012
-*/

static const char* rcsID = "$Id: my_first_separate_source.cc,v 1.1 2012/04/17 09:55:24 cvsbert Exp $";

#include "my_first_separate_source.h"
#include "uimsg.h"


My_Class::My_Class()
    : my_variable_(0)
{
    uiMSG().message( "Hello world!" );
}
