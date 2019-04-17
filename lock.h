#include "types.h"
#include "stat.h"
#include "user.h"

static inline uint
xchg(volatile uint *addr, uint newval)
{
  uint result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  uint cpu;   // The cpu holding the lock.
};

void            acquire(struct spinlock*, uint c);
int             holding(struct spinlock*, uint c);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*, uint c);