#!/bin/bash
function ergodic(){
for file in `ls $1`
do
local path="/home/cai"
local name=$file 
echo $name
#解压缩
$path/zpipe -d <$path/test/$name> $path/resourse/$name.bin
#后期处理
echo "process"
$path/changeFormat  $path/resourse/$name.bin $path/result/$name.txt

#处理完可删除zpiperesult文件夹下文件
rm $path/test/$name
done
}


count=1
while (( $count<=5))
do
INIT_PATH="/home/cai/test"
ergodic $INIT_PATH
done
