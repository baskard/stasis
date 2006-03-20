#include <lladd/graph.h>
#include <alloca.h>
#include <assert.h>
#include <stdlib.h>
#include "page.h"
#include <lladd/crc32.h>

extern int transClos_outdegree;
extern int numOut;
extern int numTset;
extern int numShortcutted;
extern int numSkipped;
extern int numPushed;
extern int useCRC;

void naiveTraverse(int xid, recordid rid, int num) {

  int * node = alloca(sizeof(int) * (transClos_outdegree+1));

  Tread(xid, rid, node);

  if(node[transClos_outdegree] == num) { return; }

  assert(node[transClos_outdegree] == (num-1));

  node[transClos_outdegree] = num;
  numTset++;
  Tset(xid, rid, node);

  int i = 0;

  for(i = 0; i < transClos_outdegree; i++) {
    rid.slot = node[i];
    naiveTraverse(xid, rid, num);
  }
}

pthread_mutex_t counters = PTHREAD_MUTEX_INITIALIZER;

/** @todo need to load the correct pages, since the local fifo doesn't refer to a single page!!! */
void multiTraverse(int xid, recordid arrayList, lladdFifo_t * local, lladdFifo_t * global, lladdFifoPool_t * pool, int num) {
  
  int * node        = alloca(sizeof(int) * (transClos_outdegree+1));
  int * nodeScratch = alloca(sizeof(int) * (transClos_outdegree+1));
  
  int myFifo = -1;
  
  int deltaNumOut = 0;
  int deltaNumSkipped = 0;
  int deltaNumShortcutted = 0;
  int deltaPushed = 0;

  while(Titerator_tryNext(xid, local->iterator)) { // @nextOrEmprty?
    recordid * rid;
    recordid localRid;
    size_t size = Titerator_value(xid, local->iterator, (byte**)&rid);
    
    assert(size == sizeof(recordid));
    
    localRid = *rid;

    if(myFifo == -1) {
      if(useCRC) {
	myFifo = crc32((byte*)&(rid->page), sizeof(rid->page), (unsigned int)-1) % pool->fifoCount;
      } else { 
	myFifo = rid->page % pool->fifoCount;
      }
      //      printf("Switched locality sets... %d\n", myFifo);
    } else { 
      //      assert(myFifo == crc32((byte*)&(rid->page), sizeof(rid->page), (unsigned long)-1L) % pool->fifoCount);
    }

    Titerator_tupleDone(xid, local->iterator);
    Tread(xid, localRid, node);

    if(node[transClos_outdegree] != num) {
      assert(node[transClos_outdegree] == (num-1));

      node[transClos_outdegree] = num;
      numTset++;
      Tset(xid, localRid, node);  /// @todo TsetRange?
      int i;
      for(i =0 ; i < transClos_outdegree; i++) { 
	recordid nextRid = arrayList;
	nextRid.slot = node[i];
	Page * p = loadPage(xid, arrayList.page); // just pin it forever and ever
	nextRid = dereferenceArrayListRid(p, nextRid.slot); 
	releasePage(p);

	int thisFifo = crc32((byte*)&(nextRid.page), sizeof(nextRid.page), (unsigned int)-1) % pool->fifoCount;
	/*	if(nextRid.page == rid->page) {
	  assert(thisFifo == myFifo);
	  }*/
	//	if(nextRid.page == localRid.page) {
	if(thisFifo == myFifo) {
	  deltaNumShortcutted++;
	  Tread(xid, nextRid, nodeScratch);
	  if(nodeScratch[transClos_outdegree] != num) {
	    Tconsumer_push(xid, local->consumer, NULL, 0, (byte*)&nextRid, sizeof(recordid));
	    deltaNumOut++;
	  } else {
	    deltaNumSkipped++;
	  } 
	} else {
	  // @todo check nextRid to see if we're the worker that will consume it, or (easier) if it stays on the same page.
	  Tconsumer_push(xid, global->consumer, NULL, 0, (byte*)&nextRid, sizeof(recordid));
	  deltaPushed++;
	  deltaNumOut++;
	}
      }
    }
    deltaNumOut--;

  }

  pthread_mutex_lock(&counters);
  numOut += deltaNumOut;
  numSkipped += deltaNumSkipped;
  numShortcutted += deltaNumShortcutted;
  numPushed += deltaPushed;
  pthread_mutex_unlock(&counters);


}
