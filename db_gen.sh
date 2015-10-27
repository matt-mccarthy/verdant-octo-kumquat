name=$1
count=$2
source=$3

bcount=`expr $count \* 28`

touch $name

dd if=$source of=$name bs=512 count=$bcount status=progress
