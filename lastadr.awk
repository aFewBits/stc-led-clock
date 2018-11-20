# match every record
{
    if (substr($0,1,1) == ":" && substr($0,8,2) == "00"){
        len = hex2dec(substr($0,2,2))
        addr = hex2dec(substr($0,4,4))
        if (highest < addr+len)
            highest = addr+len
    }
}

function hex2dec(v)
{
    slen = length(v)
    tbl = "0123456789ABCDEF"
    ov = 0
    e = 0
    for (i=slen; i>0 ; i--){
        ss1 = substr(v,i,1)
        hv = (index(tbl,ss1)-1)
        ov = ov + (hv * (16^e++))
        }
    return ov
}

END {
    printf("Highest address = %d\n",highest )
    }
