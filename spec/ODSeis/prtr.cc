#define private public
#define protected public
#include "transl.h"

int main( int, char** )
{
    BufferString msg;
    msg.add( "\nSize of TranslatorGroup: " )
		.add( sizeof(TranslatorGroup) );
    msg.add( "\nOffset of clssname_: " )
		.add( (long)(&(((TranslatorGroup*)0)->clssname_)) );
    msg.add( "\nOffset of usrname_: " )
		.add( (long)(&(((TranslatorGroup*)0)->usrname_)) );
    msg.add( "\nOffset of selhist_: " )
		.add( (long)(&(((TranslatorGroup*)0)->selhist_)) );
    msg.add( "\nSizeof clssname_: " )
		.add( sizeof(((TranslatorGroup*)0)->clssname_) );
    msg.add( "\nSizeof usrname_: " )
		.add( sizeof(((TranslatorGroup*)0)->usrname_) );
    msg.add( "\nSizeof selhist_: " )
		.add( sizeof(((TranslatorGroup*)0)->selhist_) );
    std::cout << msg << std::endl;
    return 0;
}
