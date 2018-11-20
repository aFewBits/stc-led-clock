#
# translateChars - input two seperate data files and output transliterated characters
#
# file1 format is "hexnum chars" where hexnum = hex entry index, char = "on" led segmemts
# file2 consists a stream of "abcdefgh" char lists
#
# file1 converts raw led segment letters to a translated hex number
# which is stored in an array with the characters hex number being the index
#
# file1:
#   0x30    abcdef
#   0x31    bc
#   0x32    abdeg
#   ... repeats for 127 chars total
#
# file2:
#   CL
#   AL
#   BEEP
#
# output:
#   CL      0x31,0x4A   // CL
#   AL      0x37,0x48   // AL
#   BEEP    0x6A,0x88,0x9F,0x4C // BEEP
#
BEGIN {
	while (getline <"segTable/seglist.txt" > 0){
       if ( NF == 1 ){
            ledValueNorm[ hexToString($1 "") ] = "0xFF"
            ledValueFlip[ hexToString($1 "") ] = "0xFF"
        }
        if ( NF == 2 ) {
            val  = 0;
            for ( i = 1; i <= length($2); i++){
                c = substr($2,i,1)
                if ( c == "a" ) val += 1
                if ( c == "b" ) val += 2
                if ( c == "c" ) val += 4
                if ( c == "d" ) val += 8
                if ( c == "e" ) val += 16
                if ( c == "f" ) val += 32
                if ( c == "g" ) val += 64
                if ( c == "h" ) val += 128
            }
            s = ""
            if ( val >= 128 ){ s = s "1"; val -= 128 } else { s = s "0"}
            if ( val >=  64 ){ s = s "1"; val -=  64 } else { s = s "0"}
            if ( val >=  32 ){ s = s "1"; val -=  32 } else { s = s "0"}
            if ( val >=  16 ){ s = s "1"; val -=  16 } else { s = s "0"}
            if ( val >=   8 ){ s = s "1"; val -=   8 } else { s = s "0"}
            if ( val >=   4 ){ s = s "1"; val -=   4 } else { s = s "0"}
            if ( val >=   2 ){ s = s "1"; val -=   2 } else { s = s "0"}
            if ( val >=   1 ){ s = s "1"; val -=   1 } else { s = s "0"}

        	b01 = substr(s,8,1)         # get each bit in order
        	b02 = substr(s,7,1)
        	b04 = substr(s,6,1)
        	b08 = substr(s,5,1)
        	b10 = substr(s,4,1)
        	b20 = substr(s,3,1)
        	b40 = substr(s,2,1)
        	b80 = substr(s,1,1)
        	#
        	# the bit reorder is here:
            #
            # inverted logic here!
            #
            val = 0
            if ( b80 == "0" ){ val += 128 }
            if ( b40 == "0" ){ val +=  64 }
            if ( b20 == "0" ){ val +=  32 }
            if ( b10 == "0" ){ val +=  16 }
            if ( b08 == "0" ){ val +=   8 }
            if ( b04 == "0" ){ val +=   4 }
            if ( b02 == "0" ){ val +=   2 }
            if ( b01 == "0" ){ val +=   1 }
            ledValueNorm[ hexToString($1 "") ] = sprintf("0x%02X",val)
            #
            # inverted logic here!
            #
            val = 0
            if ( b80 == "0" ){ val += 128 }
            if ( b40 == "0" ){ val +=  64 }
            if ( b04 == "0" ){ val +=  32 }
            if ( b02 == "0" ){ val +=  16 }
            if ( b01 == "0" ){ val +=   8 }
            if ( b20 == "0" ){ val +=   4 }
            if ( b10 == "0" ){ val +=   2 }
            if ( b08 == "0" ){ val +=   1 }
            ledValueFlip[ hexToString($1 "") ] = sprintf("0x%02X",val)
        }
    }
    # end of file 1

	while (getline < "segTable/srclist.txt" > 0){
        printf("\t")
        for( i=1; i<=length($1); i++){
            c = substr($1,i,1)
            if ( i == 3)
                printf("%s,",ledValueFlip[c])
            else
                printf("%s,",ledValueNorm[c])
            }
        if (length($1) >= 3)
            printf("\t// C3F %s\n",$1)
        else
            printf("\t// %s\n",$1)

        if (length($1) >= 3){
            printf("\t")
            for( i=1; i<=length($1); i++){
                c = substr($1,i,1)
                printf("%s,",ledValueNorm[c])
                }
            printf("\t// C3N %s\n",$1)
        }
	}

} # end of pgm

function hexToString(hexchars)
{
    # hexchars = "0x4F"
    msb = substr(hexchars,3,1)
    lsb = substr(hexchars,4,1)
    val = 0
    if ( msb == "F" ){ val += 240 }
    if ( msb == "E" ){ val += 224 }
    if ( msb == "D" ){ val += 208 }
    if ( msb == "C" ){ val += 192 }
    if ( msb == "B" ){ val += 176 }
    if ( msb == "A" ){ val += 160 }
    if ( msb == "9" ){ val += 144 }
    if ( msb == "8" ){ val += 128 }
    if ( msb == "7" ){ val += 112 }
    if ( msb == "6" ){ val +=  96 }
    if ( msb == "5" ){ val +=  80 }
    if ( msb == "4" ){ val +=  64 }
    if ( msb == "3" ){ val +=  48 }
    if ( msb == "2" ){ val +=  32 }
    if ( msb == "1" ){ val +=  16 }
    if ( lsb == "F" ){ val +=  15 }
    if ( lsb == "E" ){ val +=  14 }
    if ( lsb == "D" ){ val +=  13 }
    if ( lsb == "C" ){ val +=  12 }
    if ( lsb == "B" ){ val +=  11 }
    if ( lsb == "A" ){ val +=  10 }
    if ( lsb == "9" ){ val +=   9 }
    if ( lsb == "8" ){ val +=   8 }
    if ( lsb == "7" ){ val +=   7 }
    if ( lsb == "6" ){ val +=   6 }
    if ( lsb == "5" ){ val +=   5 }
    if ( lsb == "4" ){ val +=   4 }
    if ( lsb == "3" ){ val +=   3 }
    if ( lsb == "2" ){ val +=   2 }
    if ( lsb == "1" ){ val +=   1 }
    return sprintf("%c",val)
}
