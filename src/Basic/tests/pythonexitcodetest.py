import argparse
import sys

def runtest(rettype) -> bool:
    if rettype == 1:
        print( "Error argument passed" )
        print( "Fail return", file=sys.stderr )
        return False
    elif rettype > 1:
        print( "Exception argument passed" )
        raise ValueError("Exception raised.")
    
    print( "Success argument passed" )
    return True

def main() -> int:
    parser = argparse.ArgumentParser( description='Return type for test prog.' )
    parser.add_argument( 'rettype', type=int,
                         help='Integer input that determines the return type' )
    
    args = parser.parse_args()
    rettype = args.rettype
    ex_code = 0 if runtest(rettype) else 1
    return ex_code

if __name__ == "__main__":
    sys.exit( main() )
