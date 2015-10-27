modulus=$1
dir=$2
first=$3
last=$4
source=$5

(mkdir $dir) 2> /dev/null

for i in `seq $first $last`
do
	j=$(($i % $modulus))
	
	k_ones=$(($j % 10))
	k_tens=$(( $(($j/10)) % 10 ))
	k_huns=$(( $(($j/100)) % 10 ))
	
	(mkdir $dir/$k_huns $dir/$k_huns/$k_tens $dir/$k_huns/$k_tens/$k_ones) 2> /dev/null
	touch $dir/$k_huns/$k_tens/$k_ones/$i.txt
	
	(dd if=$source of=$dir/$k_huns/$k_tens/$k_ones/$i.txt bs=512 count=28) 2> /dev/null
done
