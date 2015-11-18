#
# creates 2D line from a 'Simple file' 3d cube export
# Input has to contain file header and inl/crl
# Output will contain file header and trcnr, x, y
# The line will have a sinusoid geometry
#
# You can make the corresponding PS 2D data with mk_2d_ps_from_3d_strip.awk
#

BEGIN {
    crl0 = 700; trcnr0 = 1000;
    x0 = 605000; y0 = 6080000;
    ywidth = 1500; dx = 40; nrx = 650; nrsins = 3;

    sinfac = 10 * nrsins / nrx;
    trcnr = trcnr0 - 1;
}

{
    if ( NR < 2 )
	print $0;
    else if ( $2 == crl0 )
    {
	trcnr++;
	seqnr = trcnr - trcnr0;
	x = x0 + seqnr * dx;
	y = y0 + ywidth * sin( sinfac * seqnr );

	printf( "%d\t%d\t%d", trcnr, x, y );
	for ( i=3; i<=NF; i++ )
	    printf( "\t%s", $i );
	printf( "\n" );
    }
}
