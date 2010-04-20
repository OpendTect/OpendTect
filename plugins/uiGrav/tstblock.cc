#include <iostream>
#include "grav.h"


int main( int, char** )
{
    Grav::Block blk( Coord(-100,100), Coord( 300, -100 ),
	    		Interval<double>(50,150) );

    Coord3 c3( 0, 0, 0 );

    for ( int idx=0; idx<2001; idx++ )
    {
	c3.x = -1000 + idx;
	const double val = 1e12 * blk.calcValue(c3);
	std::cout << c3.x << '\t' << val << std::endl;
    }

    return 0;
}
