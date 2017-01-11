for f in examples/*.c; do
    if ! [ -f $f.3addr ] ; then
        ../cs380c_lab1/src/csc.exe $f > ${f}.3addr
    fi
    python main.py -c ${f}.3addr | md5sum

done
