/*+
 * (C) Your_copyright
 * AUTHOR   : Your_name_here
 * DATE     : Apr 2012
-*/


#include "my_first_separate_source.h"
#include "uimsg.h"


My_Class::My_Class()
{
    uiMSG().message( toUiString("Hello world!") );
}
