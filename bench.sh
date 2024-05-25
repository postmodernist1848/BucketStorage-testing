prev=0
for (( c=50000; c<=110000; c+=2000 ))
do 
    cur=$(./main --gtest_filter=benchmark.insert_erase_iter $c | grep ms | head -1 | awk '{print substr($5, 2)}')
    echo "$c iterations:"
    echo "    $cur seconds"
    echo "    difference of $(($cur-$prev))"

    #echo -n "$c " # go to https://planetcalc.com/5992/ and approximate Big O complexity
    #echo -n "$cur "

    prev=$cur
done
echo ""
