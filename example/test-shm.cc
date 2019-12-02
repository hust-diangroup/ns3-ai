#include "ns3/train-var.h"

using namespace ns3;

struct BigVar
{
  int a[100];
} Packed;

int
main ()
{

  //Use simple variables
  //Create a variable in shared memory pool with id = 1
  TrainVar<int> var (1);
  for (int i = 0; i < 100000; ++i)
    {
      //Set the variable to 2*i
      //The Python program will wait for the set value of the code
      var.Set (2 * i);
      //Get the variable returned by the python program
      //The code will wait for the Python program return value
      int ret = var.Get ();
      NS_ASSERT_MSG (ret == 2 * i + 1, "Error");
    }

  //For complex variables (Reduce one memory copy)
  //Register the variable in memory pool with id = 2
  SharedMemoryPool::Get ()->RegisterMemory (2, sizeof (BigVar));
  for (int i = 0; i < 1000; ++i)
    {
      //Verify that the operation should be performed
      //If the version is odd, the python program operate the memory
      //else the C program operate the memory
      while (SharedMemoryPool::Get ()->GetMemoryVersion (2) % 2 != 0)
        ;
      //Acquire the variable
      //The Python program will wait until release the variable
      BigVar *bigvar = (BigVar *) SharedMemoryPool::Get ()->AcquireMemory (2);
      if (i != 0)
        for (int j = 0; j < 100; ++j)
          NS_ASSERT_MSG (bigvar->a[j] == 2 * ((i - 1) * 1000 + j), "ERROR");
      for (int j = 0; j < 100; ++j)
        bigvar->a[j] = i * 1000 + j;
      //Release memory's operation right
      SharedMemoryPool::Get ()->ReleaseMemory (2);
    }

  //Free memory pool
  SharedMemoryPool::Get ()->FreeMemory ();

  puts ("OK");
  return 0;
}