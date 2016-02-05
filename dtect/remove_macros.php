<?php

#Blanks out all C/C++ macros and replaces them with empty lines
#So line numbers remain the same. All changes are written back to the file
#so be aware!


#Check syntax
if ( count( $argv ) < 2 )
{
    echo "Syntax $argv[0] <filename>\n";
    return 1;
}

#Set to true for debug-printouts
$debug = false;

for ( $idx=count($argv)-1; $idx>0; $idx-- ) {
    #Read file
    $file = file_get_contents( $argv[$idx] );

    #Divide file into an array where each line is one entry
    $filelines = explode( "\n", $file );

    $output = "";

    $ongoingmacro = false; #Is true if we are inside a macro
    foreach ($filelines as $line) {
	if ( $ongoingmacro ) {
	    #Check if there is a \ character at the end, if not end macro
	    if ( preg_match( "/(\\\)$/", $line )===0 ) {
		$ongoingmacro = false;
		if ( $debug ) {
		    $output .= "ENDMACRO $line";
		}
	    }
	    else if ( $debug ) {
		$output .= "INMACRO $line";
	    }

	}
	else {
	    //Is a macro starting
	    if ( preg_match( '/^#define.*/', $line )===1 ) {
		if ( $debug ) {
		    $output .= "BEGINMACRO $line";
		}

		//Is there a \ character at the end, so it is multi-line
		if ( preg_match( "/(\\\)$/", $line )===1 )
		    $ongoingmacro = true;
	    }
	    else {
		if ( $debug )
		    $output .= "NORMAL ";
	    
		$output .= "$line";
	    }
	}

	$output .= "\n";
    }

    if ( $debug )
	echo $output;
    else
	file_put_contents( $argv[$idx], $output );
}
