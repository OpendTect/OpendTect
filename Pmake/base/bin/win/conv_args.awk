BEGIN { FS=" "; isprog=0; outfil=""; link=1; linkflags=""; compileflags="" }
{

    for( i=1; i<= NF; i++ )
    {
        if( $i ~ /^-I/ )
        {
            keep = $0;

            tmp = substr($i,3);
            if( tmp ) {tmp = "cygpath -w " tmp; }
            else { i++; tmp = "cygpath -w " $(i); }

            if( tmp | getline )
                compile_flags = compile_flags " -I" $0;
            close( tmp );
            $0=keep;
        }
        else if( $i ~ /^-L/ )
        {
            keep = $0;

            tmp = substr($i,3);
            if( tmp ) {tmp = "cygpath -w " tmp; }
            else { i++; tmp = "cygpath -w " $(i); }

            if( tmp | getline )
                compile_flags = compile_flags " -L" $0;
            close( tmp );
            $0=keep;
        }
        else if( $i ~ /^-o/ )
        {
            keep = $0;

            tmp = substr($i,3);
            if( tmp ) {tmp = "cygpath -w " tmp; }
            else { i++; tmp = "cygpath -w " $(i); }

            if( tmp | getline )
                compile_flags = compile_flags " -o " $0;
            close( tmp );
            $0=keep;
        }
        else if( $i ~ /^\/c\// || $i ~ /^\/cygdrive/  || $i ~ /^\/tmp/ )
        {
            keep = $0;

            tmp = "cygpath -w " $(i);
            if( tmp | getline )
                compile_flags = compile_flags " " $0;
            close( tmp );

            $0=keep;
	}
        else if( $i ~ /=\/cygdrive/ )
        {
            keep = $0;
	    curword = $i;
	    len = length( curword );
	    start = index( $i, "=/cygdrive");
	    head =  substr(curword,0,start);
	    tail =  substr(curword,start+1);

            tmp = "cygpath -w " tail;
            if( tmp | getline )
                compile_flags = compile_flags " " head $0;
            close( tmp );

            $0=keep;
	}
	else { compile_flags = compile_flags " " $i ;}
    }
    printf( compile_flags );
}

