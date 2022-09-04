There are 4 test mode to test api execution time:
#define SPIN_LOCK_0_M           0 //just test spin_lock()
#define SPIN_LOCK_IRQSAVE_1_M   1 //just test spin_lock_irqsave()
#define SPIN_MIX_01_M           2 //first test spin_lock() and then spin_lock_irqsave()
#define SPIN_MIX_10_M           3 //first test spin_lock_irqsave() and then spin_lock()

Due to the presence of multiple cores and big.LITTLE cores, it's best to use taskset to bind the cores so that the results from each test are not too different.
You can use command:

taskset 1 echo "21000" > /proc/my_driver && dmesg | grep "my_driver"

"21000":
   2: mode 2, first test spin_lock() and then spin_lock_irqsave().
1000: number of loops. In each mode, you should use a larger number of loops, such as 100,1000,10000...

A small number of loops may result in inaccurate test results because the API itself has a short execution time and is easily influenced by the testing method. As the number of loops becomes large, the impact of the test method becomes small. And the greater the number of loops, the more accurate the test results.
