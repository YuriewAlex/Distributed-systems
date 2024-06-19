export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/user/pa4start";
export LD_PRELOAD="/home/user/pa4start/libruntime.so";
clang -std=c99 -Wall -pedantic *.c -o pa2 -L. -lruntime


count="$1"
mtx="$2"
if [ "$count" -lt 2 ] || [ "$count" -gt 10 ]; then
  echo "Неверное число. Введите число от 2 до 10."
  exit 1
fi





export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/user/pa4start"; 

LD_PRELOAD=/home/user/pa4start/libruntime.so ./pa2 -p $count $mtx
