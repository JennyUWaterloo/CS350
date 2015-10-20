#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

struct lock *intersectionLock;
struct cv *generalCv;

volatile int count = 0;
volatile int curOrigin = -1;
volatile int curDestination = -1;
volatile int intersection[4][4];

void
intersection_sync_init(void)
{
	intersectionLock = lock_create("intersectionLock");
	
	if (intersectionLock == NULL) {
		panic("Problem initiating intersectionLock");
	}

	 generalCv = cv_create("generalCv");

	 if (generalCv == NULL) {
	 	panic("Problem initiating intersectionCv");
	 }
	 
	for (int i = 0; i < 4; i++){
		for (int j = 0; j < 4; j++) {

			intersection[i][j] = 0;
		}
	}
}

void
intersection_sync_cleanup(void)
{
	lock_destroy(intersectionLock);
	cv_destroy(generalCv);
}

/*
 * enum Directions
 * 	north = 0
 *	east = 1
 *	south = 2
 *	west = 3
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
	KASSERT(intersectionLock != NULL);
    KASSERT(generalCv != NULL);

	lock_acquire(intersectionLock);
	// kprintf("origin: %d\n", origin);
	// kprintf("destination: %d\n", destination);
	// kprintf("currently in intersection:\n");

	if (curOrigin == -1) curOrigin = origin;
	if (curDestination == -1) curOrigin = destination;

	if (origin == 0) {
		if (destination == 1) {
			while (curOrigin != 0 || curDestination != 1 ||
				intersection[0][1] > 0 || intersection[0][2] > 0 ||intersection[0][3] > 0 ||
				intersection[1][0] > 0 ||
				intersection[3][2] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);
				} else {
					curOrigin = 0;
					curDestination = 1;
				}
			}
		}

		else if (destination == 2) {
			while (curOrigin != 0 || curDestination != 2 ||
				intersection[0][1] > 0 || intersection[0][2] > 0 ||intersection[0][3] > 0 ||
				intersection[1][0] > 0 ||
				intersection[2][0] > 0 || intersection[2][1] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 0;
					curDestination = 2;
				}
			}
		}

		else if (destination == 3) {
			while (curOrigin != 0 || curDestination != 3 ||
				intersection[0][1] > 0 || intersection[0][2] > 0 ||intersection[0][3] > 0 ||
				intersection[1][0] > 0 ||
				intersection[2][1] > 0 ||
				intersection[3][2] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 0;
					curDestination = 3;
				}			}
		}
	}

	//East (1)

	else if (origin == 1) {
// kprintf("1\n");
		if (destination == 0) {
			while (curOrigin != 1 || curDestination != 0 ||
				intersection[1][0] > 0 || intersection[1][2] > 0 ||intersection[1][3] > 0 ||
				intersection[0][3] > 0 ||
				intersection[2][1] > 0 ||
				intersection[3][2] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 1;
					curDestination = 0;
				}
			}
		}
	
		else if (destination == 2) {
			while (curOrigin != 1 || curDestination != 2 ||
				intersection[1][0] > 0 || intersection[1][2] > 0 ||intersection[1][3] > 0 ||
				intersection[0][3] > 0 ||
				intersection[2][1] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 1;
					curDestination = 2;
				}
			}
		}

		else if (destination == 3) {
			while (curOrigin != 1 || curDestination != 3 ||
				intersection[1][0] > 0 || intersection[1][2] > 0 ||intersection[1][3] > 0 ||
				intersection[2][1] > 0 ||
				intersection[3][2] > 0 || intersection[3][1] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 1;
					curDestination = 3;
				}
			}
		}
	}

	//South (2)

	else if (origin == 2) {
// kprintf("2\n");
		if (destination == 0) {
			while (curOrigin != 2 || curDestination != 0 ||
				intersection[2][0] > 0 || intersection[2][1] > 0 ||intersection[2][3] > 0 ||
				intersection[0][3] > 0 || intersection[0][2] > 0 ||
				intersection[3][2] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 2;
					curDestination = 0;
				}
			}
		}

		else if (destination == 1) {
			while (curOrigin != 2 || curDestination != 1 ||
				intersection[2][0] > 0 || intersection[2][1] > 0 ||intersection[2][3] > 0 ||
				intersection[0][3] > 0 ||
				intersection[1][0] > 0 || 
				intersection[3][2] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 2;
					curDestination = 1;
				}
			}
		}

		else if (destination == 3) {
			while (curOrigin != 2 || curDestination != 3 ||
				intersection[2][0] > 0 || intersection[2][1] > 0 ||intersection[2][3] > 0 ||
				intersection[1][0] > 0 ||
				intersection[3][2] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 2;
					curDestination = 3;
				}
			}
		}
	}

	//West (3)

	else if (origin == 3) {
// kprintf("3\n");
		if (destination == 0) {
			while (curOrigin != 3 || curDestination != 0 ||
				intersection[3][0] > 0 || intersection[3][1] > 0 ||intersection[3][2] > 0 ||
				intersection[0][3] > 0 ||
				intersection[2][1] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 3;
					curDestination = 0;
				}
			}
		}

		else if (destination == 1) {
			while (curOrigin != 3 || curDestination != 1 ||
				intersection[3][0] > 0 || intersection[3][1] > 0 ||intersection[3][2] > 0 ||
				intersection[0][3] > 0 ||
				intersection[1][0] > 0 || intersection[1][3] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 3;
					curDestination = 1;
				}
			}
		}

		else if (destination == 2) {
			while (curOrigin != 3 || curDestination != 2 ||
				intersection[3][0] > 0 || intersection[3][1] > 0 ||intersection[3][2] > 0 ||
				intersection[0][3] > 0 ||
				intersection[1][0] > 0 ||
				intersection[2][1] > 0) {
				if (count != 0) {
					cv_wait(generalCv, intersectionLock);	
				} else {
					curOrigin = 3;
					curDestination = 2;
				}
			}
		}
	}


	intersection[origin][destination]++;
	count++;
	cv_signal(generalCv, intersectionLock);
	// kprintf("let %d %d in intersection\n", origin, destination);
	lock_release(intersectionLock);
// kprintf("end of entry");
}

void
intersection_after_exit(Direction origin, Direction destination) 
{
	lock_acquire(intersectionLock);
	while (count == 0) {
		cv_wait(generalCv, intersectionLock);
	}
	intersection[origin][destination]--;
	count--;
	cv_signal(generalCv, intersectionLock);
	lock_release(intersectionLock);
}
