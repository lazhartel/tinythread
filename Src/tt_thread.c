#include "../Inc/tt_thread.h"

#define TCB_AT(thread_buf_addr,thread_buf_size) \
	(((uint32_t)(thread_buf_addr) + (thread_buf_size) - sizeof (TT_THREAD_T)) / 4 * 4)

static unsigned int	irq_stack[TT_IRQ_STACK_SIZE / sizeof(unsigned int)];
static LIST_T		g_all_threads;
static LIST_T		g_athread_running[TT_THREAD_PRIORITY_NUM];
TT_THREAD_T			*g_thread_current;
TT_THREAD_T			*g_thread_next;
static TT_THREAD_T	g_thread_main;
//static uint32_t		g_thread_nop_buffer[TT_THREAD_BUFFER_SIZE (8) / sizeof (uint32_t)];

//static void tt_create_thread_nop (void);
static void tt_init_thread_main (void);



static void tt_use_PSP(unsigned int irq_stack_addr)
{
	__set_PSP(__get_MSP());
	__set_MSP(irq_stack_addr);
	__set_CONTROL(2);
	__ISB();
}


void tt_init (uint32_t systick_frequency)
{
	int i;

	sysDisableIRQ();
    
    tt_use_PSP((unsigned int)irq_stack + sizeof(irq_stack));
			
	listInit (&g_all_threads);
	for (i = 0; i < TT_THREAD_PRIORITY_NUM; i++)
	{
		listInit (&g_athread_running[i]);
	}
	g_thread_current = NULL;
	//tt_create_thread_nop ();
	tt_init_thread_main ();

	/* Enable timer to start schedule. */
	tt_timer_init (systick_frequency);
    sysEnableIRQ();
	
#if 0	/* Useless code! */	
	/* Call tt_exit_thread after main(). */
	atexit (tt_thread_exit);
#endif

#ifdef	TT_SUPPORT_USLEEP
	tt_enable_usleep ();
#endif	// TT_SUPPORT_SLEEP
}


/* Available in: irq, thread. */
TT_THREAD_T *tt_thread_self (void)
{
	return g_thread_current;
}


#if 0
static void __tt_thread_entry (void)
{
	TT_THREAD_T *current_thread = tt_thread_self ();
	(*current_thread->tt_thread_entry) (current_thread->tt_thread_arg);
	tt_thread_exit ();
}
#endif


static void tt_schedule_remove (void *arg)
{
#ifdef	TT_SUPPORT_DUMP_THREAD
	listDetach (&g_thread_current->list_threads);
#endif	// TT_SUPPORT_STACK_CHECK

	listDetach (&g_thread_current->list_schedule);
	g_thread_current->wait_parent = NULL;

#ifdef	TT_SUPPORT_MUTEX
	listDetach (&g_thread_current->list_wait_head);
	listDetach (&g_thread_current->list_wait_node);
#endif	// TT_SUPPORT_MUTEX

	__tt_schedule ();
}


static void tt_link_thread_to_system(const char *name, TT_THREAD_T *thread)
{
	memcpy (thread->name, name, sizeof (thread->name));
	thread->name[sizeof (thread->name) - 1] = '\0';

	sysDisableIRQ ();

#ifdef	TT_SUPPORT_MUTEX
	thread->priority	= thread->fixed_priority;
	listInit (&thread->list_wait_head);
	listInit (&thread->list_wait_node);
	thread->locked_by_mutex = false;
#endif	// TT_SUPPORT_MUTEX

	listInit (&thread->list_schedule);
	listMove (&g_athread_running[thread->priority], &thread->list_schedule);
	
	thread->wait_parent = &g_athread_running;

#ifdef	TT_SUPPORT_DUMP_THREAD
	listInit (&thread->list_threads);
	listMove (&g_all_threads, &thread->list_threads);
#endif	// TT_SUPPORT_STACK_CHECK


	sysEnableIRQ ();
	
	//sysSafePrintf ("Create thread: %08x %s\n", thread, name);
}

/* Available in: irq, thread. */
TT_THREAD_T *tt_thread_create (
	const char		*name,
	unsigned char	priority,
	void			*buffer,	/* Global */
	size_t			buffer_size,
	void			(*thread_entry) (void *),
	void			*thread_arg
	)
{
	uint32_t				stack_base;
	uint32_t				stack_limit;
	TT_THREAD_T				*thread;
 	TT_THREAD_PUSH_STACK	*stack;
	
	if (buffer == NULL)
		return NULL;	//No buffer
	
	//if (priority < 0)
	//	priority = 0;
	if (priority >= TT_THREAD_PRIORITY_NUM)
		priority = TT_THREAD_PRIORITY_NUM - 1;
	
	/* The buffer layout
	     (stack_limit) <-- stack area --> (stack_base) <-- TT_THREAD_T --> (buffer_end)
	 */
	stack_base	= TCB_AT(buffer, buffer_size);
	stack_limit	= (uint32_t) buffer;
	thread		= (TT_THREAD_T *)stack_base; 
	
	if (stack_base <= stack_limit)
		return NULL;	//No stack size

#ifdef	TT_SUPPORT_MUTEX
	thread->fixed_priority	= priority;
#else	// TT_SUPPORT_MUTEX
	thread->priority = priority;
#endif	// TT_SUPPORT_MUTEX

#ifdef	TT_SUPPORT_STACK_CHECK
	thread->stack_base		= (void *) stack_base;
	thread->stack_max_used	= thread->stack_base;
	thread->stack_limit		= (void *) stack_limit;
#endif	// TT_SUPPORT_STACK_CHECK

	thread->uSP = stack_base - sizeof(TT_THREAD_PUSH_STACK);
		
	stack = (TT_THREAD_PUSH_STACK*)thread->uSP;
	//0xFFFFFFFDUL is the LR for interrupt handler return to thread with PSP
	stack->uIRQ_LR = 0xFFFFFFFDUL;
	stack->uR0 = (uint32_t) thread_arg;
	stack->uPC = ((uint32_t) thread_entry & ~0x1UL);
	stack->uLR = ((uint32_t) tt_thread_exit & ~0x1UL) + 0x1UL;
	stack->uPSR = 0x01000000UL;	//Thumb mode

	tt_link_thread_to_system(name, thread);

	return thread;
}



/* Available in: thread. */
void tt_thread_exit (void)
{
	tt_syscall (NULL, tt_schedule_remove);
}


/* Available in: thread. */
void tt_thread_yield (void)
{
	tt_syscall (NULL, __tt_schedule_yield);
}

#if 0
static void tt_thread_nop_entry (void *arg)
{
	while (1)
	{
#if 1
		__WFI();
#else
		static int nop_count = 0;
		//sysSafePrintf ("In nop = %d!\n", nop_count++);
#endif
	}
}

static void tt_create_thread_nop (void)
{
	TT_THREAD_T *thread = tt_thread_create ("__tt_nop", 0, g_thread_nop_buffer, sizeof (g_thread_nop_buffer),
		tt_thread_nop_entry, NULL);
	listDetach (&thread->list_schedule);
	thread->wait_parent = NULL;
	//sysSafePrintf("%d\n", sizeof(g_thread_nop_buffer));
}
#endif


static void tt_init_thread_main (void)
{
	TT_THREAD_T *thread = &g_thread_main;

	//sysSafePrintf("tt_main = %x\n", thread);
	//thread->thread_entry	= 0;
	//thread->thread_arg	= 0;

#ifdef	TT_SUPPORT_MUTEX	
	thread->fixed_priority	= 0;
#else	// TT_SUPPORT_MUTEX
	thread->priority = 0;
#endif	// TT_SUPPORT_MUTEX

#ifdef	TT_SUPPORT_STACK_CHECK
	thread->stack_base		= (void *) 0;
	thread->stack_max_used	= thread->stack_base;
	thread->stack_limit		= (void *) 0;
#endif	// TT_SUPPORT_STACK_CHECK

	tt_link_thread_to_system("__tt_main", thread);

	g_thread_current = thread;
	g_thread_next = thread;
}


#ifdef	TT_SUPPORT_STACK_CHECK
static void tt_check_stack(TT_THREAD_T *thread)
{
	if (thread == NULL)
		return;

	//Calculate max stack used.
	if ((uint32_t) thread->stack_max_used == 0
		|| (uint32_t) thread->stack_max_used > thread->uSP)
		thread->stack_max_used = (void *) thread->uSP;

	//Check stack overflow
	if ((uint32_t) thread->stack_base > 0)	/* For main thread, do not check this. */
	{
		if (thread->uSP < (uint32_t) thread->stack_limit)
		{
			sysSafePrintf ("Stack down overflow for task: (%08x, %08x) %s\n", thread, thread->uSP, thread->name);
			/* Indicate stack down overflow if run to here. */
			while(1);
		}
		
		if (thread->uSP > (uint32_t) thread->stack_base - sizeof(TT_THREAD_PUSH_STACK))
		{
			sysSafePrintf ("Stack up overflow for task: (%08xk, %08x) %s\n", thread, thread->uSP, thread->name);
			/* Indicate stack up overflow if run to here,
			   It rarely happens unless the program is damaged. */
			while(1);
		}
	}
}
#endif	// TT_SUPPORT_STACK_CHECK

/* Available in: irq. */
void __tt_schedule (void)
{
	unsigned char	priority;
	
#ifdef	TT_SUPPORT_STACK_CHECK
	/* Check if thread stack is healty */
	tt_check_stack(g_thread_current);
#endif	// TT_SUPPORT_STACK_CHECK

#ifdef	TT_SUPPORT_SLEEP
	__tt_wakeup ();
#endif	// TT_SUPPORT_SLEEP

	for (priority = 0; priority < TT_THREAD_PRIORITY_NUM; priority++)
	{
		if (g_athread_running[priority].pNext != &g_athread_running[priority])
		{
			/* Run next thread. */
			LIST_T *pNext = g_athread_running[priority].pNext;
			//sysSafePrintf("leng: %d\n", listLength(&g_athread_running[priority]));
			//listMove (&g_athread_running[priority], pNext);
			g_thread_next = GetParentAddr (pNext, TT_THREAD_T, list_schedule);
			return;
		}
	}
	
	/* Run to here means no thread is running! */
	g_thread_next = NULL;
	//g_thread_next = (TT_THREAD_T *) TCB_AT(g_thread_nop_buffer, sizeof(g_thread_nop_buffer));
	//sysSafePrintf ("current: (%08x) %s\n", 	g_thread_current, g_thread_current->name);
	//sysSafePrintf ("%08x %08x %08x\n", g_thread_current->uPC, g_thread_current->uLR, g_thread_current->uSP);
}


/* Available in: irq. */
void tt_set_thread_running (TT_THREAD_T *thread)
{
	listMove (&g_athread_running[thread->priority], &thread->list_schedule);
	thread->wait_parent = &g_athread_running;
}

void __tt_schedule_yield (void *arg)
{
	if(g_thread_current != NULL)
	{
		listMove (g_thread_current->list_schedule.pPrev, &g_thread_current->list_schedule);
		//listSwapBefore (&g_thread_current->list_schedule);
	}	
	__tt_schedule ();
}


typedef struct
{
	TT_THREAD_T		*thread;
	unsigned char	priority;
#ifdef	TT_SUPPORT_MUTEX
	bool			inherit;
#endif	// TT_SUPPORT_MUTEX
} __SET_PRIORITY_T;


static void __tt_set_priority (void *arg)
{
	__SET_PRIORITY_T	*set_priority_arg	= (__SET_PRIORITY_T *)arg;
	TT_THREAD_T			*thread				= set_priority_arg->thread;
	unsigned char		priority			= set_priority_arg->priority;

	if (thread->priority != priority)
	{
		//sysSafePrintf("set priority of thread %x from %d to %d\n",
		//		thread, thread->priority,
		//		priority);

		if (thread->wait_parent == &g_athread_running)
			listMove (&g_athread_running[priority], &thread->list_schedule);
		thread->priority = priority;
	}

#ifdef	TT_SUPPORT_MUTEX
	if (set_priority_arg->inherit == false)
		thread->fixed_priority = priority;
#endif	// TT_SUPPORT_MUTEX
}

#ifdef	TT_SUPPORT_MUTEX
void tt_set_priority_inherit (TT_THREAD_T *thread, unsigned char priority)
{
	__SET_PRIORITY_T set_priority_arg;
	set_priority_arg.thread		= thread;
	set_priority_arg.priority	= priority;
	set_priority_arg.inherit	= true;
	__tt_set_priority ((void *) &set_priority_arg);

	//sysSafePrintf("Set inherit priority for %x to %d\n", thread, (int)priority);
}
#endif	// TT_SUPPORT_MUTEX

void tt_set_priority (TT_THREAD_T *thread, unsigned char priority)
{
	__SET_PRIORITY_T set_priority_arg;
	set_priority_arg.thread		= thread;
	set_priority_arg.priority	= priority;
#ifdef	TT_SUPPORT_MUTEX
	set_priority_arg.inherit	= false;
#endif	// TT_SUPPORT_MUTEX
	tt_syscall ((void *) &set_priority_arg, __tt_set_priority);
}

unsigned char tt_get_priority (TT_THREAD_T *thread)
{
#ifdef	TT_SUPPORT_MUTEX
	return thread->fixed_priority;
#else	// TT_SUPPORT_MUTEX
	return thread->priority;
#endif	// TT_SUPPORT_MUTEX
}


#ifdef	TT_SUPPORT_DUMP_THREAD
void tt_dump_threads (void (*func_dump) (TT_THREAD_T *thread, void *arg), void *arg)
{
	LIST_T *list;
	sysDisableIRQ ();
	
	for (list = g_all_threads.pNext; list != &g_all_threads; list = list->pNext)
	{
		TT_THREAD_T *thread = GetParentAddr (list, TT_THREAD_T, list_threads);
		(*func_dump) (thread, arg);
	}
	
	sysEnableIRQ ();
}
#endif	// TT_SUPPORT_DUMP_THREAD 

