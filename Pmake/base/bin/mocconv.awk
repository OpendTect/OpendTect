BEGIN { FS=" ";}
{
    for( i=1; i<= NF; i++ )
    {
	if( $i ~ /\.h/  || $i ~ /\.H/ ) 
	{ 
	    keep = $0;
	    tmp = "cygpath -w " $(i);

	    if( tmp | getline ) 
		printf (" %s", $0 ); 
	    close( tmp );

	    $0=keep;
	}
	else printf(" %s", $i);
    }
}
