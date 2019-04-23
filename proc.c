#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define MAXCONT 8
#define MAXINODE 50

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct barrier_type
{
    int nproc;
    int counter; // initialize to 0
    volatile int flag; // initialize to 0
    struct spinlock lock;
};

struct barrier_type b;
static struct proc *initproc;

int nextpid = 1;
int sched_log=0;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

/* Container table defined here */
struct cont{
  int valid;
  int cid;
  int inodes[MAXINODE];
  char* name_mapping_in[MAXINODE];  //filename in container
  char* name_mapping_out[MAXINODE]; //global filename
  int num_name_mapping;
  int numproc;
  int cmalloc[8];
  void* cmalloc_map[8];
};

struct c_table {
  struct spinlock lock;
  struct cont cont[MAXCONT];
} ctable;


void
cinit(void)
{
  initlock(&ctable.lock, "ctable");
}

/*******************************/

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");
  p->qhead = -1;
  p->qtail = -1;
  p->rec_busy = 0;
  p->sig_handle_set = 0;
  p->sig_received = 0;
  p->sig_msg = (char*)kalloc();
  p->local_sense = 0;
  p->cid = 0;

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  np->qtail = -1;
  np->qhead = -1;
  np->rec_busy = 0;
  np->sig_handle_set = 0;
  np->sig_received = 0;
  np->sig_msg = (char*)kalloc();
  np->local_sense = 0;
  np->amicontainer=0;
  np->cid=0;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  ctable.cont[0].numproc++;
  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  ctable.cont[curproc->cid].numproc--;

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
int contsched=0;
struct proc *lastproccont[MAXCONT];
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  int flag=0;
  int i;
  lastproccont[0]=ptable.proc;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();
    acquire(&ptable.lock);
    if(flag==0)
    {
      for (int i = 0; i < 8; ++i){
            contsched=(contsched+1)%8;
            if (ctable.cont[contsched].valid == 1 && ctable.cont[contsched].numproc >0){
              break;
            }
      }
    }
    flag=0;
    p = lastproccont[contsched];


    for(i=0; i<NPROC; i++){
      p++;
      if (p == &ptable.proc[NPROC])
      {
        p=ptable.proc;
      }
      if(p->state != RUNNABLE||p->cid!=contsched){
        continue;
      }
      flag=1;
      lastproccont[contsched]=p;

      if(sched_log==1)
      {
        cprintf("Container + %d : Scheduling process + %d\n", contsched, p->pid );
      }

      
      for (int i = 0; i < 8; ++i){
        contsched=(contsched+1)%8;
          if (ctable.cont[contsched].valid == 1 && ctable.cont[contsched].numproc >0){
            break;
          }
      }


      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      if(p->sig_handle_set && p->sig_received){
        //cprintf("signal handler called: %s\n", p->sig_msg);
        p->sig_handle(p->sig_msg);
        //cprintf("signal handler returned\n");
        p->sig_received = 0;
      }

      swtch(&(c->scheduler), p->context);

      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


int 
send_signal(int sender_pid, void* msg, struct proc* p)
{
  if(p->sig_handle_set){
    p->sig_received = 1;
    memmove(p->sig_msg, msg, MSGSIZE);
  }
  return 0;
}

//lists all currently running processes
int
ps(void)
{
  int cid = myproc()->cid;
  struct proc *p;

  acquire(&ptable.lock);

  cprintf("container:%d\n", cid);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->cid == cid && (p->state == RUNNABLE || p->state == RUNNING || p->state == SLEEPING))
      cprintf("pid:%d name:%s\n", p->pid, p->name);

  release(&ptable.lock);
  return 0;
}

//send unicast message
int
send(void)
{
  int sender_pid, rec_pid;
  char* msg;
  if(argint(0, &sender_pid) < 0 || argint(1, &rec_pid) < 0 || argptr(2, &msg, MSGSIZE) < 0)
    return -1;    //cannot read arguments

  //cprintf("send :%s pid:%d %d\n", msg,sender_pid,rec_pid);

  struct proc *p = 0;
  struct proc *temp;

  acquire(&ptable.lock);

  for(temp = ptable.proc; temp < &ptable.proc[NPROC]; temp++){
    if(temp->pid == sender_pid && temp->state == UNUSED){
      release(&ptable.lock);
      cprintf("send: invalid sender id : %d\n", sender_pid);
      return -2;    //invalid sender_id
    }
    if(temp->pid == rec_pid){
      if(temp->state == UNUSED){
        release(&ptable.lock);
        cprintf("send: invalid receiver id\n");
        return -3;    //invalid rec_id
      }
      p = temp;
    }
  }
  if(p == 0){
    release(&ptable.lock);
    cprintf("send: invalid receiver id\n");
    return -3;    //invalid rec_id
  }

  if((p->qtail + 1)%MAX_Q_LEN == p->qhead){
    cprintf("send: message queue full, sender:%d, receiver:%d\n", sender_pid, rec_pid);
    release(&ptable.lock);
    return -4;    //message queue full
  }  

  if(p->qhead == -1 && p->qtail == -1){
    p->qhead = 0;
    p->qtail = 0;
  } else {
    p->qtail = (p->qtail + 1)%MAX_Q_LEN;
  }

  memmove(p->msgq[p->qtail], msg, MSGSIZE);

  // reciever might be waiting
  wakeup1(p);

  release(&ptable.lock);

  return 0;
}

//recieve unicast message
int
recv(void)
{
  struct proc *p = myproc();
  char* msg;
  if(argptr(0, &msg, MSGSIZE) < 0)
    return -1;    //cannot read arguments

  acquire(&ptable.lock);

  while(p->qhead == -1 && p->qtail == -1){
    sleep(p, &ptable.lock);   //wait for sender
  }
  if(p->qhead == p->qtail){
    memmove(msg, p->msgq[p->qhead], MSGSIZE);
    p->qtail = -1;
    p->qhead = -1;
  } else {
    memmove(msg, p->msgq[p->qhead], MSGSIZE);
    p->qhead = (p->qhead + 1)%MAX_Q_LEN;
  }

  release(&ptable.lock);
  return 0;
}



//send multicast message
int
send_multi(void)
{
  int sender_pid;
  int length;
  char* rec_pids;
  char* msg;
  if(argint(0, &sender_pid) < 0 || argstr(1, &rec_pids) < 0 || argstr(2, &msg) < 0 || argint(3, &length) < 0)
    return -1;    //cannot read arguments

  int* rec_list = (int*)rec_pids;

  struct proc *p;

  acquire(&ptable.lock);

  for(int i = 0; i < length; i++){
    int rec_pid = rec_list[i];
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->pid == rec_pid){
        if(p->state == UNUSED || p->state == EMBRYO || p->state == ZOMBIE){
          //invalid receiver id (Handle?)
        } else{
          send_signal(sender_pid, msg, p);
        }
      }
    }
  }

  release(&ptable.lock);

  //cprintf("send_multi :%s pid:%d %d\n", msg,sender_pid,rec_list[0]);

  return 0;
}

//set signal handler
int set_handle(void)
{
  //cprintf("set handle called\n");
  struct proc *curproc = myproc();
  void (*handle)(void* msg);
  if(argptr(0, (void*)&handle, sizeof(handle)) < 0)
    return -1;    //cannot read arguments
  curproc->sig_handle = handle;
  curproc->sig_handle_set = 1;
  return 0;
}

//set barrier
int set_barrier(void)
{
  int nproc;
  if(argint(0, &nproc) < 0)
    return -1;    //cannot read arguments
  initlock(&b.lock, "barrier");
  b.nproc = nproc;
  b.counter = 0;
  b.flag = 0;
  //cprintf("barrier set: %d\n", b.nproc);
  return 0;
}

//barrier
int barrier(void)
{
  int bnum;
  if(argint(0, &bnum) < 0)
    return -1;    //cannot read arguments

  struct proc *curproc = myproc();
  
  //cprintf("barrier called: %d\n", curproc->pid);

  acquire(&b.lock);
  curproc->local_sense = 1 - curproc->local_sense;
  b.counter++;
  int arrived = b.counter;
  if (arrived == b.nproc) // last arriver sets flag
  {
      release(&b.lock);
      b.counter = 0;
      // memory fence to ensure that the change to counter
      // is seen before the change to flag
      b.flag = curproc->local_sense;
      //cprintf("barrier flag: %d\n", b.flag);
  }
  else
  {
      release(&b.lock);
      while (b.flag != curproc->local_sense); // wait for flag
  }
  //cprintf("barrier returned: %d:%d\n", curproc->pid, b.flag);
  return 0;
}

int create_container(void)
{
  int cid = 0;
  for (int i = 1; i < MAXCONT; ++i)
  {
    if(ctable.cont[i].valid == 0)
    {
      ctable.cont[i].valid = 1;
      ctable.cont[i].cid = i;
      ctable.cont[i].numproc = 0;
      lastproccont[i]=ptable.proc;

      for(int j = 0; j < MAXINODE; j++){
        ctable.cont[i].inodes[j] = ctable.cont[0].inodes[j];
      }
      ctable.cont[i].num_name_mapping = 0;
      cid = i;

      for (int i = 0; i < 8; ++i)
      {
        ctable.cont[i].cmalloc[i]=-1;
        ctable.cont[cid].cmalloc_map[i]=kalloc();
      }
      break;
    }
  }
  return cid;
}

int join_container(int cid)
{
  struct proc *p;
  
  acquire(&ptable.lock);
  
  p = myproc();
  ctable.cont[p->cid].numproc--;
  p->cid = cid;
  ctable.cont[cid].numproc++;

  release(&ptable.lock);
  
  return 0;
}


int leave_container()
{
  struct proc *p;
  
  acquire(&ptable.lock);
  
  p = myproc();
  ctable.cont[p->cid].numproc--;
  p->cid = 0;
  ctable.cont[0].numproc++;
  
  release(&ptable.lock);
  
  return 0;
}


int destroy_container(int cid)
{
  ctable.cont[cid].valid = 0;

  struct proc *p;
  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->cid == cid){
      p->cid = 0;
    }
  }

  release(&ptable.lock);
  return 0;
}

void scheduler_log_on(void)
{
  sched_log=1;
}

void scheduler_log_off(void)
{
  sched_log=0;
}

// extern struct proc* myproc(void);
// extern ctable;
// int
// container_malloc(uint nbytes)
// {
//   void* allocated=0;
//   // void* allocated=malloc(nbytes);
//   int cid=myproc()->cid;
//   int i=0;
//   while(ctable.cont[cid].cmalloc[i]!=-1)
//   {
//     i++;
//   }
  
//   ctable.cont[cid].cmalloc_map[i]=allocated;

//   return i;

// }
